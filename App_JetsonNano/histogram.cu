#include <cuda.h>
#include <iostream>
#include <fstream>
#include <opencv4/opencv2/core/hal/interface.h>
#include <opencv4/opencv2/core/matx.hpp>
#include <opencv4/opencv2/highgui.hpp>
#include <opencv4/opencv2/imgcodecs.hpp>
#include <opencv4/opencv2/opencv.hpp>
#include <opencv4/opencv2/imgproc.hpp>
#include <opencv4/opencv2/core/core.hpp>
#include <chrono>

struct IN_TYPE {
    char r;
    char g;
    char b;
};

#define NUM_BINS 256
#define NUM_PARTS 256*3

#define NUM_SUB_HIST_1 5
#define NUM_SUB_HIST_2 50

__global__ void histogram_gmem_atomics(const IN_TYPE *in, int width, int height, unsigned int *out)
{

   //printf("%d %d %d\n", blockIdx.x, blockIdx.y, blockIdx.z);
   // pixel coordinates
  int x = blockIdx.x * blockDim.x + threadIdx.x;
  int y = blockIdx.y * blockDim.y + threadIdx.y;

  // grid dimensions
  int nx = blockDim.x * gridDim.x; 
  int ny = blockDim.y * gridDim.y;

  // linear thread index within 2D block
  int t = threadIdx.x + threadIdx.y * blockDim.x; 

  // total threads in 2D block
  int nt = blockDim.x * blockDim.y; 
  
  // linear block index within 2D grid
  int g = blockIdx.x + blockIdx.y * gridDim.x;

  // initialize temporary accumulation array in global memory
  unsigned int *gmem = out + g * NUM_PARTS;
  for (int i = t; i < 3 * NUM_BINS; i += nt) gmem[i] = 0;

 // printf("x: %d y: %d nx:%d ny:%d t:%d nt:%d g:%d w:%d h:%d\n", x,y,nx,ny,t,nt,g,width,height);

  // process pixels
  // updates our block's partial histogram in global memory
  for (int col = x; col < width; col += nx) 
    for (int row = y; row < height; row += ny) { 
      char r = (in[row * width + col].r);
      char g = (in[row * width + col].g);
      char b = (in[row * width + col].b);
      //printf("r:%d g:%d b:%d\n", r, g, b);
      atomicAdd(&gmem[NUM_BINS * 0 + r], 1);
      atomicAdd(&gmem[NUM_BINS * 1 + g], 1);
      atomicAdd(&gmem[NUM_BINS * 2 + b], 1);
    }
}

__global__ void histogram_final_accum(const unsigned int *in, int n, unsigned int *out)
//__global__ void histogram_final_accum()
{
  //printf("Something");
  //printf("%d %d %d\n", blockIdx.x , blockDim.x , threadIdx.x);
  int i = blockIdx.x * blockDim.x + threadIdx.x;
  if (i < 3 * NUM_BINS) {
    unsigned int total = 0;
    for (int j = 0; j < n; j++) 
      total += in[i + NUM_PARTS * j];
    
    out[i] = total;
    //out[i] = 57;
  }else
    out[i] = 101;
  
}

unsigned int* histogram_gpu(int num_threads_x, int num_threads_y, IN_TYPE *pixel_array, int cols, int rows){
    unsigned int *out, *out2;

    cudaMallocManaged(&out,  sizeof(unsigned int)*3*NUM_BINS*num_threads_x*num_threads_y);
    cudaMallocManaged(&out2, sizeof(unsigned int)*3*NUM_BINS);


    histogram_gmem_atomics<<<num_threads_x,num_threads_y>>>(pixel_array, cols, rows, out);
    cudaDeviceSynchronize();

    histogram_final_accum<<<3,255>>>(out, num_threads_x*num_threads_y, out2);
    cudaDeviceSynchronize();
    cudaFree(out);
    return out2;
}

void histogram_cpu(IN_TYPE *pixel_array, int cols, int rows, int* hist){

    for (int i = 0; i < cols; i++)
    {
        for(int j = 0; j<rows;j++)
        {
            char r = ( pixel_array[j * cols + i].r);
            char g = ( pixel_array[j * cols + i].g);
            char b = ( pixel_array[j * cols + i].b);
	    hist[NUM_BINS * 0 + r]++;
	    hist[NUM_BINS * 1 + g]++;
	    hist[NUM_BINS * 2 + b]++;
        }
    }

}

int main(void)
{
    // File to write the Histogram into
    std::ofstream histFile ("histogram.txt");
    cv::Mat inputFile = cv::imread("hist_rainbow.png", cv::IMREAD_COLOR);

    IN_TYPE *pixel_array;

    IN_TYPE *pixel_array_cpu = (IN_TYPE*)malloc(sizeof(IN_TYPE)*inputFile.cols*inputFile.rows);

    cudaMallocManaged(&pixel_array,  sizeof(IN_TYPE)*inputFile.cols*inputFile.rows);

    

    std::cout << "BEGINING COLOR CHANNELS ..." << std::endl;
    // Variable to store the byte from each channel
    cv::Vec3b byteFromPixel;

    int pixel_idx = 0;


    // Get all the bytes from the red channel 

    for (int i = 0; i < inputFile.rows; i++)
    {
        for(int j = 0; j<inputFile.cols;j++)
        {
            // Store the byte in each corresponding file according to channel
            byteFromPixel = inputFile.at<cv::Vec3b>(i,j);
            // byteArray[byteFromPixel]++;
    
            pixel_array[pixel_idx].b = byteFromPixel[0];
            pixel_array[pixel_idx].g = byteFromPixel[1];
            pixel_array[pixel_idx].r = byteFromPixel[2];

            pixel_array_cpu[pixel_idx].b = byteFromPixel[0];
            pixel_array_cpu[pixel_idx].g = byteFromPixel[1];
            pixel_array_cpu[pixel_idx].r = byteFromPixel[2];
            pixel_idx++;
        }
    }

    std::chrono::steady_clock::time_point begin, end;

    int n_threads[9] = {1, 2, 4, 8, 16, 32, 64, 128, 256};
    unsigned int* hist;


    for(int i = 0; i < 9; i++){
	for (int j = i; j < 9; j++){
	       begin = std::chrono::steady_clock::now();

	       hist = histogram_gpu(n_threads[j], n_threads[i], pixel_array, inputFile.cols, inputFile.rows);

	       end = std::chrono::steady_clock::now();

	       printf("Done run GPU (%3dx%3d threads): %8d us\n",n_threads[j], n_threads[i], std::chrono::duration_cast<std::chrono::microseconds>(end-begin));
		cudaFree(hist);
	}
    }

    int hist_cpu[NUM_BINS*3] = {};
    begin = std::chrono::steady_clock::now();

    histogram_cpu( pixel_array_cpu, inputFile.cols, inputFile.rows, hist_cpu);

    end = std::chrono::steady_clock::now();

    printf("Done run cpu:\nTime: %d ms\n\n", std::chrono::duration_cast<std::chrono::microseconds>(end-begin));

    for(int k = 0; k < 3;k++)
    {
        for(int i = 0; i < NUM_BINS; i++){
            // Print
           histFile << hist_cpu[k*NUM_BINS+i] << std::endl;
        }
    }

    cudaFree(pixel_array);
}
