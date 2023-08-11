#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <png.h>
#ifdef OMP_USE
#include <omp.h>
#endif

#ifndef __KNNIMDENOISER_H
#define __KNNIMDENOISER_H

#define KNN_VERSION       "1.1"
#define FILTER_RADIUS     3
#define THRESHOLD_WEIGHT  0.02f
#define THRESHOLD_LERP    0.66f
#define NOISE_VAL         0.0f // get stdev image, test value = 0.32f
#define NOISE_COEF        3.0f
#define NOISE_EPS         0.0000001f
#define LERPC             0.16f
#define THREADSCOUNT      2

png_bytep* read_png_file(char *filename, int *png_size);
void write_png_file(char *filename, png_bytep *row_pointers, int width, int height);
void image_to_array(png_byte *image, png_bytep *row_pointers, int width, int height);
void array_to_image(png_byte *image, png_bytep *row_pointers, int width, int height);
float colorDistance(float* a, float* b);
float pixelDistance(float x, float y);
float lerpf(float a, float b, float c);
float image_stdev(png_byte *img, int width, int height, int filter_radius);
void knn_filter(png_byte *img, png_byte *img_out, int width, int height, int filter_radius, float threshold_weight, float threshold_lerp, float noise_val, float lerpc);

#endif /* __KNNIMDENOISER_H */
