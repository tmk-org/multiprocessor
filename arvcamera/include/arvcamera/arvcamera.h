#ifndef _ARV_CAMERA_H_
#define _ARV_CAMERA_H_

#define SOURCE_PATH_ENV "TEST_SOURCE_PATH"

#if 0 // {TESTDB_ROOT} / 20000
    #define DATA_CNT 13
    #define IMAGE_HEADER_SIZE sizeof(size_t)
    #define IMAGE_RAW_WIDTH 2448
    #define IMAGE_RAW_HEIGHT 2048
    #define IMAGE_RAW_DEPTH 8
    #define IMAGE_RAW_SIZE ( IMAGE_RAW_WIDTH * IMAGE_RAW_HEIGHT * IMAGE_RAW_DEPTH / 8 )
    #define DATA_SIZE ( IMAGE_HEADER_SIZE + IMAGE_RAW_SIZE )
#else // {TESTDB_ROOT} / pencil_frames_5
    #define DATA_CNT 873
    #define IMAGE_HEADER_SIZE ( sizeof(size_t) + sizeof(int) )
    #define IMAGE_RAW_WIDTH 960
    #define IMAGE_RAW_HEIGHT 1920
    #define IMAGE_RAW_CHANNELS 3
    #define IMAGE_RAW_DEPTH 8
    #define IMAGE_RAW_SIZE ( IMAGE_RAW_WIDTH * IMAGE_RAW_HEIGHT * IMAGE_RAW_CHANNELS * IMAGE_RAW_DEPTH / 8 )
    #define DATA_SIZE ( IMAGE_HEADER_SIZE + IMAGE_RAW_SIZE )
#endif

#endif //_ARV_CAMERA_H_
