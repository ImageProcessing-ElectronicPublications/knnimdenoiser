# knnimdenoiser

KNN image denoise is image denoising using K Nearest Neighbors filter.

## Overview

This application uses a K Nearest Neighbors filter to remove noise from images.
The project was based on [this paper](https://developer.download.nvidia.com/compute/cuda/1.1-Beta/x86_website/projects/imageDenoising/doc/imageDenoising.pdf).

## Compilation

You can compile the application using a command:

```
make
```

## Running the application

In order to run the program, you need to specify two arguments and options:

```shell
usage:
./knnimdenoiser [options] in.png out.png

options:
  -c N.N    noise coefficient (default 3.000000)
  -l N.N    lerpc (default 0.160000)
  -n N.N    noise (default 0.000000)
  -p NUM    threads count (default 2)
  -r NUM    radius (default 3)
  -t N.N    threshold lerp (default 0.660000)
  -w N.N    threshold weight (default 0.020000)
  -h        show this help message and exit
```

For example:

```shell
./knnimdenoiser input.png output.png
```
## Example images before and after denoising process:

See [knnimdenoiser-demo](https://github.com/ImageProcessing-ElectronicPublications/knnimdenoiser-demo):

Noisy image                |  Fixed image
:-------------------------:|:-------------------------:
![](https://raw.githubusercontent.com/ImageProcessing-ElectronicPublications/knnimdenoiser-demo/main/images/noise/medusa_noise.png)  |  ![](https://raw.githubusercontent.com/ImageProcessing-ElectronicPublications/knnimdenoiser-demo/main/images/fixed/medusa_fixed.png)
![](https://raw.githubusercontent.com/ImageProcessing-ElectronicPublications/knnimdenoiser-demo/main/images/noise/portrait_noise.png)  |  ![](https://raw.githubusercontent.com/ImageProcessing-ElectronicPublications/knnimdenoiser-demo/main/images/fixed/portrait_fixed.png)
![](https://raw.githubusercontent.com/ImageProcessing-ElectronicPublications/knnimdenoiser-demo/main/images/noise/yoda_noise.png)  |  ![](https://raw.githubusercontent.com/ImageProcessing-ElectronicPublications/knnimdenoiser-demo/main/images/fixed/yoda_fixed.png)
