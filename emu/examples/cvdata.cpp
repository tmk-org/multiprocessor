#include <stdio.h>
#include <string.h>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

#define TEST_DATA_CNT  1
#define TEST_HEADER_SIZE sizeof(size_t)
#define TEST_RAW_WIDTH 2448
#define TEST_RAW_HEIGHT 2048
#define TEST_RAW_DEPTH 8
#define TEST_RAW_SIZE ( TEST_RAW_WIDTH * TEST_RAW_HEIGHT * TEST_RAW_DEPTH / 8 )
#define TEST_DATA_SIZE ( TEST_HEADER_SIZE + TEST_RAW_SIZE )

int main(){
    char data[TEST_DATA_CNT][TEST_DATA_SIZE];
    char *buf = NULL;
    size_t data_size = TEST_DATA_SIZE;
    printf("data_size = %d\n", data_size);
    int i, j;
    for (i = 0; i < TEST_DATA_CNT; i++) {
        char *chunk = (char*)((char*)data + i * TEST_DATA_SIZE); 
        //memcpy((void*)chunk, (void*)&data_size, TEST_HEADER_SIZE);
        *((size_t*)chunk) = data_size;
        char name[256];
        sprintf(name, "../data/test_%c.bmp", 65 + i);
        cv::Mat orig = cv::imread(name, cv::IMREAD_ANYDEPTH |
                                                  cv::IMREAD_ANYCOLOR);
        int origSize = orig.total() * orig.elemSize();
        if (origSize != TEST_RAW_SIZE) {
            fprintf(stdout, "[cvdata]: problems with datasize\n");
            return 0;
        }
        memcpy(chunk + TEST_HEADER_SIZE, (char*)orig.data, TEST_RAW_SIZE);
        fprintf(stdout, "[cvdata]: chunk_size = %zd\n", *((size_t*)chunk));
        //---------------------------------------------------------------
        cv::Mat img;
        img = cv::Mat::zeros(TEST_RAW_HEIGHT, TEST_RAW_WIDTH, CV_8UC1);
        int imgSize = img.total() * img.elemSize();
        buf = (char*)((char*)data + i * TEST_DATA_SIZE);
        data_size = *(size_t*)buf;
        fprintf(stdout, "[cvdata]: res_size = %zd\n", data_size);
        if (imgSize != data_size - TEST_HEADER_SIZE) {
            fprintf(stdout, "[cvdata]: problems with result size\n");
            //return 0;
        }
        img.data = (uchar *)(buf + TEST_HEADER_SIZE);
        sprintf(name, "../data/test_%c_res.bmp", 65 + i);
        cv::imwrite(name, img);
    }
    return 0;
}
