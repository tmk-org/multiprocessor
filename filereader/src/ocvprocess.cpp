#include "filereader/imageprocess.h"

#include <string.h>
#include <stdio.h>

#define EXTERNAL_IMAGE_PROCESS
//#undef EXTERNAL_IMAGE_PROCESS

#ifdef EXTERNAL_IMAGE_PROCESS

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

#include <iostream>

struct image_t {
    size_t size;
    int width;
    int height;
    //void *img_ptr; 
    //cv::Mat img;
};

int read_image(char **pdata, char *src){
    if (!pdata) return -1;
    if (!(*pdata)) {
        *pdata = (struct image_t*)malloc(sizeof(struct image_t));
    }
    struct image_t *image = (struct image_t*)(*pdata);
    //TODO: place for work with image
    cv::Mat img = cv::imread(src, cv::IMREAD_ANYDEPTH |
                                                  cv::IMREAD_ANYCOLOR);
    image->size = img.total() * img.elemSize();
    image->width
    std::cout << "[read]: " << src << " ImgSize = " << image->size << std::endl; 
    std::cout << "[read]: " << src << " ImgSize = " << image->size << std::endl; 
    std::cout << "[read]: " << src << " ImgSize = " << image->size << std::endl; 
    //image->img_ptr = (void*)(&img);
    //image->img = img;
    *pdata = (char*)image;
    std::cout << "!" << endl;
    return 0;
}

int write_image(char *data, const char *dst){
    struct image_t *image = (struct image_t*)data;
    //TODO: place for work with image
    //cv::Mat img = *((cv::Mat *)(image->img_ptr));
    cv::Mat img = image->img;
    int imgSize = img.total() * img.elemSize();
    std::cout << "[write]: ImgSize = " << imgSize << std::endl; 
    cv::imwrite(dst, img);
    free(image);
    return 0;
}

#endif
