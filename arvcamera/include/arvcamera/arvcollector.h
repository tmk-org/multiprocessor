#ifndef _ARV_COLLECTOR_H_
#define _ARV_COLLECTOR_H_

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
    char source[256]; //source identification as string
};

struct data_collector *collector_init(char* source);
void collector_destroy(struct data_collector *collector);

#endif //_ARV_COLLECTOR_H_
