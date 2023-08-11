#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <png.h>
#ifdef OMP_USE
#include <omp.h>
#endif
#include "knnimdenoiser.h"

void knn_filter_usage(char* prog)
{
    printf("KNN image denoise version %s.\n", KNN_VERSION);
    printf("usage: %s [options] in.png out.png\n", prog);
    printf("options:\n");
    printf("  -c N.N    noise coefficient (default %f)\n", NOISE_COEF);
    printf("  -l N.N    lerpc (default %f)\n", LERPC);
    printf("  -n N.N    noise (default %f)\n", NOISE_VAL);
    printf("  -p NUM    threads count (default %d)\n", THREADSCOUNT);
    printf("  -r NUM    radius (default %d)\n", FILTER_RADIUS);
    printf("  -t N.N    threshold lerp (default %f)\n", THRESHOLD_LERP);
    printf("  -w N.N    threshold weight (default %f)\n", THRESHOLD_WEIGHT);
    printf("  -h        show this help message and exit\n");
}

png_bytep* read_png_file(char *filename, int *png_size)
{
    int width, height;
    png_byte color_type;
    png_byte bit_depth;
    png_bytep *row_pointers = NULL;
    FILE *fp = fopen(filename, "rb");

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(!png) abort();

    png_infop info = png_create_info_struct(png);
    if(!info) abort();

    if(setjmp(png_jmpbuf(png))) abort();

    png_init_io(png, fp);

    png_read_info(png, info);

    width      = png_get_image_width(png, info);
    height     = png_get_image_height(png, info);
    color_type = png_get_color_type(png, info);
    bit_depth  = png_get_bit_depth(png, info);
    png_size[0] = width;
    png_size[1] = height;

    // Read any color_type into 8bit depth, RGBA format.
    // See http://www.libpng.org/pub/png/libpng-manual.txt

    if(bit_depth == 16)
        png_set_strip_16(png);

    if(color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png);

    // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
    if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png);

    if(png_get_valid(png, info, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png);

    // These color_type don't have an alpha channel then fill it with 0xff.
    if(color_type == PNG_COLOR_TYPE_RGB ||
            color_type == PNG_COLOR_TYPE_GRAY ||
            color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

    if(color_type == PNG_COLOR_TYPE_GRAY ||
            color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png);

    png_read_update_info(png, info);

    if (row_pointers) abort();

    row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
    for(int y = 0; y < height; y++)
    {
        row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png,info));
    }

    png_read_image(png, row_pointers);

    fclose(fp);

    png_destroy_read_struct(&png, &info, NULL);

    return row_pointers;
}

void write_png_file(char *filename, png_bytep *row_pointers, int width, int height)
{
    FILE *fp = fopen(filename, "wb");
    if(!fp) abort();

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) abort();

    png_infop info = png_create_info_struct(png);
    if (!info) abort();

    if (setjmp(png_jmpbuf(png))) abort();

    png_init_io(png, fp);

    // Output is 8bit depth, RGBA format.
    png_set_IHDR(
        png,
        info,
        width, height,
        8,
        PNG_COLOR_TYPE_RGBA,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
    );
    png_write_info(png, info);

    // To remove the alpha channel for PNG_COLOR_TYPE_RGB format,
    // Use png_set_filler().
    //png_set_filler(png, 0, PNG_FILLER_AFTER);

    if (!row_pointers) abort();

    png_write_image(png, row_pointers);
    png_write_end(png, NULL);

    for(int y = 0; y < height; y++)
    {
        free(row_pointers[y]);
    }
    free(row_pointers);

    fclose(fp);

    png_destroy_write_struct(&png, &info);
}

void image_to_array(png_byte *image, png_bytep *row_pointers, int width, int height)
{
    int y, x;
    unsigned long int l;
    png_byte *row;
    png_byte *px;

    for(y = 0; y < height; y++)
    {
        row = row_pointers[y];
        for(x = 0; x < width; x++)
        {
            px = &(row[x * 4]);
            l = (y * width + x) * 4;
            image[l] = px[0];
            image[l + 1] = px[1];
            image[l + 2] = px[2];
            image[l + 3] = px[3];
        }
    }
}

void array_to_image(png_byte *image, png_bytep *row_pointers, int width, int height)
{
    int y, x;
    unsigned long int l;
    png_byte *row;
    png_byte *px;

    for(y = 0; y < height; y++)
    {
        row = row_pointers[y];
        for(x = 0; x < width; x++)
        {
            px = &(row[x * 4]);
            l = (y * width + x) * 4;
            px[0] = image[l];
            px[1] = image[l + 1];
            px[2] = image[l + 2];
            px[3] = image[l + 3];
        }
    }
}

float colorDistance(float* a, float* b)
{
    float dr, dg, db, d;

    dr = (b[0] - a[0]) / 255.0f;
    dg = (b[1] - a[1]) / 255.0f;
    db = (b[2] - a[2]) / 255.0f;
    d = dr * dr + dg * dg + db * db;

    return d;
}

float pixelDistance(float x, float y)
{
    float d;

    d = x * x + y * y;

    return d;
}

float lerpf(float a, float b, float c)
{
    float d;

    d = a + (b - a) * c;

    return d;
}

void errorexit(const char *s)
{
    printf("\n%s",s);
    exit(EXIT_FAILURE);
}

float image_stdev(png_byte *img, int width, int height, int filter_radius)
{
    int y, x, n, i, j, xf, yf;
    unsigned long int l, lf;
    float sum, dev, sumdev, sumdevl, gray, grayf;

    l = 0;
    sumdev = 0.0f;
    for (y = 0; y < height; y++)
    {
        sumdevl = 0.0f;
        for (x = 0; x < width; x++)
        {
            gray = ((float)img[l] + (float)img[l + 1] + (float)img[l + 2]) / 765.0f;

            n = 0;
            sum = 0.0f;
            for (i = -filter_radius; i <= filter_radius; i++)
            {
                yf = y + i;
                
                if (yf < 0 || yf >= height)
                    continue;

                for (j = -filter_radius; j <= filter_radius; j++)
                {
                    xf = x + j;
                    
                    if (xf < 0 || xf >= width)
                        continue;

                    lf = (yf * width + xf) * 4;
                    grayf = ((float)img[lf] + (float)img[lf + 1] + (float)img[lf + 2]) / 765.0f;
                    sum += grayf;
                    n++;
                }               
            }
            sum = (n > 0) ? (sum / n) : sum;
            dev = ((gray - sum) * (gray - sum));

            sumdevl += dev;
            l += 4;
        }
        sumdev += sumdevl;
    }
    sumdev /= (height * width);
    sumdev = sqrt(sumdev);

    return sumdev;
}

void knn_filter(png_byte *img, png_byte *img_out, int width, int height, int filter_radius, float threshold_weight, float threshold_lerp, float noise_val, float lerpc)
{
    int filter_area;
    float filter_area_inv, noise_weight;
    int threadId = 0, threadCount = 1;

    filter_area = ((2 * filter_radius + 1) * (2 * filter_radius + 1));
    filter_area_inv = (1.0f / (float)filter_area);
    noise_weight = (1.0f / (noise_val * noise_val));
#ifdef OMP_USE
    threadId = omp_get_thread_num();
    threadCount = omp_get_num_threads();
#endif
    for (int idy = 0; idy < height; idy++)
    {
        for (int idx = threadId; idx < width; idx += threadCount)
        {

            // Normalized counter for the weight threshold
            float fCount = 0.0f;

            // Total sum of pixel weights
            float sum_weights = 0.0f;

            // Result accumulator
            float color[] = { 0.0f, 0.0f, 0.0f };

            // Center of the filter
            long pos = (idy * width + idx) * 4;
            float color_center[] = { (float)img[pos], (float)img[pos + 1], (float)img[pos + 2], (float)img[pos + 3] };

            for (int y = -filter_radius; y <= filter_radius; y++)
            {
                int idyf = idy + y;

                if (idyf < 0 || idyf >= height)
                    continue;

                for (int x = -filter_radius; x <= filter_radius; x++)
                {
                    int idxf = idx + x;

                    if (idxf < 0 || idxf >= width)
                        continue;

                    long curr_pos = (idyf * width + idxf) * 4;
                    float color_xy[] = { (float)img[curr_pos], (float)img[curr_pos + 1], (float)img[curr_pos + 2], (float)img[curr_pos + 3] };

                    float pixel_distance = pixelDistance((float)x, (float)y);

                    float color_distance = colorDistance(color_center, color_xy);

                    // Denoising
                    float weight_xy = expf(-(pixel_distance * filter_area_inv + color_distance * noise_weight));

                    color[0] += color_xy[0] * weight_xy;
                    color[1] += color_xy[1] * weight_xy;
                    color[2] += color_xy[2] * weight_xy;

                    sum_weights += weight_xy;
                    fCount += (weight_xy > threshold_weight) ? filter_area_inv : 0;
                }
            }

            // Normalize result color
            sum_weights = 1.0f / sum_weights;
            color[0] *= sum_weights;
            color[1] *= sum_weights;
            color[2] *= sum_weights;

            float lerpQ = (fCount > threshold_lerp) ? lerpc : 1.0f - lerpc;

            color[0] = lerpf(color[0], color_center[0], lerpQ);
            color[1] = lerpf(color[1], color_center[1], lerpQ);
            color[2] = lerpf(color[2], color_center[2], lerpQ);

            // Result to memory
            img_out[pos] = (png_byte)color[0];
            img_out[pos + 1] = (png_byte)color[1];
            img_out[pos + 2] = (png_byte)color[2];
            img_out[pos + 3] = img[pos + 3];
        }
    }
}

int main(int argc, char **argv)
{
    int filter_radius = FILTER_RADIUS;
    float threshold_weight = THRESHOLD_WEIGHT;
    float threshold_lerp = THRESHOLD_LERP;
    float noise_val = NOISE_VAL;
    float noise_coef = NOISE_COEF;
    float lerpc = LERPC;
    int no_threads = THREADSCOUNT;
    int fhelp = 0;
    int opt;

    while ((opt = getopt(argc, argv, ":c:l:n:p:r:t:w:h")) != -1)
    {
        switch(opt)
        {
        case 'c':
            noise_coef = atof(optarg);
            break;
        case 'l':
            lerpc = atof(optarg);
            break;
        case 'n':
            noise_val = atof(optarg);
            break;
        case 'p':
            no_threads = atoi(optarg);
            if (no_threads < 1)
            {
                fprintf(stderr, "Bad argument\n");
                fprintf(stderr, " no_threads = %d\n", no_threads);
                return 1;
            }
            break;
        case 'r':
            filter_radius = atoi(optarg);
            if (filter_radius < 1)
            {
                fprintf(stderr, "Bad argument\n");
                fprintf(stderr, " filter_radius = %d\n", filter_radius);
                return 1;
            }
            break;
        case 't':
            threshold_lerp = atof(optarg);
            break;
        case 'w':
            threshold_weight = atof(optarg);
            break;
        case 'h':
            fhelp = 1;
            break;
        case ':':
            fprintf(stderr, "ERROR: option needs a value\n");
            return 2;
            break;
        case '?':
            fprintf(stderr, "ERROR: unknown option: %c\n", optopt);
            return 3;
            break;
        }
    }
    if(optind + 2 > argc || fhelp)
    {
        knn_filter_usage(argv[0]);
    }
    else
    {
        char *src_name = argv[optind];
        char *dst_name = argv[optind + 1];
        png_bytep *row_pointers = NULL;
        int width, height;
        int png_size[2] = {0};

        // read png file to an array
        row_pointers = read_png_file(src_name, png_size);
        width = png_size[0];
        height = png_size[1];

        printf("Size: %dx%d\n", width, height);
        // allocate memory
        int size = width * height * sizeof(png_byte) * 4;

        png_byte* input_img = (png_byte*)malloc(size);
        if (!input_img) errorexit("Error allocating memory for input");

        png_byte* output_img = (png_byte*)malloc(size);
        if (!output_img) errorexit("Error allocating memory for output");

        // copy image array to allocated memory
        image_to_array(input_img, row_pointers, width, height);

        noise_val = (noise_val > 0.0f) ? noise_val : image_stdev(input_img, width, height, filter_radius);
        noise_val *= noise_coef;
        noise_val = (noise_val > NOISE_EPS) ? noise_val : NOISE_EPS;
        printf("Noise: %f\n", noise_val);

        // execute the algorithm
#ifdef OMP_USE
        #pragma omp parallel num_threads(no_threads)
#endif
        knn_filter(input_img, output_img, width, height, filter_radius, threshold_weight, threshold_lerp, noise_val, lerpc);

        // prepare array to write png
        array_to_image(output_img, row_pointers, width, height);

        // release resources
        free(output_img);
        free(input_img);

        // write array to new png file
        write_png_file(dst_name, row_pointers, width, height);

        printf("Success.\n");
    }

    return 0;
}
