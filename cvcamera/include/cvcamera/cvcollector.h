#ifndef _CV_COLLECTOR_H_
#define _CV_COLLECTOR_H_

#include <semaphore.h>
#include <pthread.h>

struct data_collector {
    int exit_flag;
    sem_t data_ready;
    pthread_t thread_id;
    union {
        int fd;
        char addr[12];
        char port[8];
    };
};

struct data_collector *collector_init(char* addr, char *port);
void collector_destroy(struct data_collector *collector);

#endif //_CV_COLLECTOR_H_
