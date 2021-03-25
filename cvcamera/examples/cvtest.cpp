#include <stdio.h>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

#include <iostream>

int main() {
    cv::Mat img = cv::imread("test.bmp", cv::IMREAD_ANYDEPTH |
                                                  cv::IMREAD_ANYCOLOR);
    int imgSize = img.total() * img.elemSize();
    std::cout << "ImgSize: " << imgSize << std::endl; 
    cv::imwrite("test_res.bmp", img);
    return 0;
}
