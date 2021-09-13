#ifndef _LOG_H_
#define _LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

extern int log_fd;

#ifndef DEBUG
    #define DEBUG 1
#endif

const char *log_fname = "/tmp/multiproc.log";

#define LOG_FD(...) if (DEBUG) { \
    if (DEBUG == 1) { fprintf(stderr, ##__VA_ARGS__); fflush(stderr); } \
    else {if (log_fd == -1) log_fd = open(log_fname, O_CREAT | O_TRUNC | O_RDWR, 0664); \
    if (log_fd != -1) dprintf(log_fd, ##__VA_ARGS__);} \
}

#ifdef __cplusplus
}
#endif

#endif
