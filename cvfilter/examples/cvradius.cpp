#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core/types.hpp>

#include <iostream>

#include "cvfilter/cvfilter.h"

int main() {
    cv::Mat orig = cv::imread("../data/pencil_frames_5/10.jpg", cv::IMREAD_ANYDEPTH |
                                                  cv::IMREAD_ANYCOLOR);
    int origSize = orig.total() * orig.elemSize();
    std::cout << "origSize: " << origSize << std::endl; 

    float radius = findRadius(orig);
    std::cout << "Radius: " << radius << std::endl;

#if 0
    cv::imwrite("../data/origin.bmp", orig);
#endif

    return 0;
}
