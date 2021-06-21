#ifndef _IMAGE_PROCESS_H_
#define _IMAGE_PROCESS_H_

#ifdef __cplusplus
extern "C" {
#endif

int read_image(char **pdata, char *src);
int write_image(char *data, const char *dst);

#ifdef __cplusplus
}
#endif

#endif 
