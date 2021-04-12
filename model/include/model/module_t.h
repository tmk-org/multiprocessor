#ifndef _MODULE_T_H
#define _MODULE_T_H

#ifndef __RING_BUFFER_H__
#include "misc/ring_buffer.h"
#endif

#ifdef __cplusplus
extern "C"
#endif
typedef struct module{
    char **argv; //command string for start module
    void *shmem;
    int id;       //idx of process in the process table 
    struct process_info	*proc_info;
    struct ring_buffer	*data_ring;
    pthread_t   thread_id;
    void * (*thread_method)(void*); //source, processor(filter) or destination
    sem_t new_cmd;
    int cmd;           //next action for module
    size_t dataoffset; //offset to data from start of shared memory for the next action
    sem_t result;   
    int state;         //resulting state of last executed action 
    struct module *next; //for  sending commands to next module
    int *exit_flag;
} module_t;

#endif //_MODULE_T_H
