#include "filereader/imageprocess.h"

#ifndef EXTERNAL_IMAGE_PROCESS

#include <string.h>
#include <stdio.h>

struct image_t {
    int x;
};

int read_image(char **pdata, char *src){
    if (!pdata) return -1;
    struct image_t *image = (struct image_t*)(*pdata);
    //TODO: place for work with image
    *pdata = (char*)image;
    return 0;
}

int write_image(char *data, const char *dst){
    struct image_t *image = (struct image_t*)data;
    //TODO: place for work with image
    return 0;
}

#endif

