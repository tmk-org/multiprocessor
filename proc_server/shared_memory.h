#ifndef _DCL_SHARED_MEM_H
#define _DCL_SHARED_MEM_H

#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#if 0
#include "memory_data.h"

typedef struct shm_pdesc {
    size_t shm_data_offset; //???
    int shm_result;         //???
    int status; //for update
    //may be IPC for capturer instead of queue for event/notify
    sem_t sem_event; 
    sem_t sem_notify;
} shm_pdesc_t;

struct shared_memory {
    char memory_map[MEMORY_MAP_SIZE];
    shm_pdesc_t pdesc[MAX_PDESC_NUM];
    memory_data_t data[MAX_DATA_SIZE];
};

#endif

#include "memory_map.h"

void* shared_memory_server_init(const char *shmpath, memory_map_t *map);
void* shared_memory_client_init(const char *shmpath, size_t shmsize);
void shared_memory_destroy(const char *shmpath);
//int shared_memory_init (int is_child);
//void shared_memory_destroy (int is_child);

#endif //_DCL_SHARED_MEM_H

