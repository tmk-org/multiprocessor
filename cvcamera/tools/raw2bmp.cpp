#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

#include <iostream>

cv::Mat restore_image(size_t rows, size_t cols, int channels, void *frame_data){
    int cv_const = CV_8UC4;
    switch (channels) {
        case 1: 
            cv_const = CV_8UC1;
            break;
        case 3:
            cv_const = CV_8UC3;
            break;
        default:
            cv_const = CV_8UC4;
        }
        cv::Mat img(rows, cols, cv_const, frame_data);
    return img;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage:\n");
        printf("\t%s in.raw out.bmp\n", argv[0]);
        return 0;
    }
    //cv::Mat img = cv::imread(argv[1], cv::IMREAD_ANYDEPTH |
    //                                              cv::IMREAD_ANYCOLOR);
    struct stat st;
    stat(argv[1], &st);
    char *frame_data = (char*)malloc(st.st_size);
    int in_fd = open(argv[1], O_RDONLY, 0600);
    if (in_fd > 0) {
        read(in_fd, frame_data, st.st_size);
        close(in_fd);
    }
    cv::Mat r_img = restore_image(960, 1920, 1, (void*)frame_data);
    cv::imwrite(argv[2], r_img);
    return 0;
}
