#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "shmem/shared_memory_map.h"
#include "pmngr/process_mngr.h"
#include "misc/ring_buffer.h"

#include "streamer/streamer.h"

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

void *destination_thread(void *data){
    module_t	*module = data;
    size_t		packetoffset;
    char		*packet;
    int cmd;

    module->id = -1;
#if 0 //no child process, only thread
    module->proc_info = (struct process_info *)(module->shmem + memory_get_processoffset(module->shmem)) + module->id;   
#endif
    while(1){
	    sem_wait(&module->new_cmd);
        cmd = module->cmd; //TODO: queue
        packetoffset = module->dataoffset;
        sem_post(&module->result);
	    if (cmd == PROCESS_CMD_STOP){
	        // previous process stop their job, finish pipeline
	        break;
	    }
        packet = (char*)(module->shmem + packetoffset);
        fprintf(stdout, "[destination_thread]: get packet %p, packet: %s\n", packet, packet);
#if 0 //no child process, only thread
        module->proc_info->cmd = cmd; 
        module->proc_info->cmd_offs = packetoffset;
	    sem_post(&module->proc_info->sem_job);
        sem_wait(&module->proc_info->sem_result);

	    if (module->proc_info->cmd_result != 0){
	        // previous process report an error, drop frame and continue
	        printf("destination_thread: destination error %d, drop packet %p\n",
		        module->proc_info->cmd_result, packet);
            //pushfree buffer
	        ring_buffer_push(module->data_ring, packetoffset); 
	        continue;
	    }
	    printf("destination_thread: passed packet %p, packet: %s\n", packet, packet);
#endif
        ring_buffer_push(module->data_ring, packetoffset);
    }
    fprintf(stdout, "[destination_thread]: stop\n");
#if 0 //no child process, only thread
	module->proc_info->cmd = PROCESS_CMD_STOP;
	sem_post(&module->proc_info->sem_job);
    sem_wait(&module->proc_info->sem_result);
#endif
    return NULL;
}

void *source_thread(void *data){
    module_t	*module = data;
    size_t		packetoffset;
    char		*packet;
    int cmd;
 
    module->id = process_start(module->argv);
    module->proc_info = (struct process_info *)(module->shmem + memory_get_processoffset(module->shmem)) + module->id;   

    while(!(*(module->exit_flag))){
        cmd = module->cmd; //TODO: queue
        packetoffset = ring_buffer_pop(module->data_ring);
        packet = module->shmem + packetoffset;
        fprintf(stdout, "[source_thread]: capture packet %p, packet %s\n", packet, packet);
        module->proc_info->cmd = cmd; 
        module->proc_info->cmd_offs = packetoffset;
	    sem_post(&module->proc_info->sem_job);
        sem_wait(&module->proc_info->sem_result);
	    fprintf(stdout, "[source_thread]: captured packet %p, packet: %s\n", packet, packet);
	    sem_wait(&module->next->result);
	    module->next->cmd = PROCESS_CMD_JOB;
	    module->next->dataoffset = packetoffset;
	    sem_post(&module->next->new_cmd);
    }
    
    fprintf(stdout, "[source_thread]: stop\n");
	sem_wait(&module->next->result);
	module->next->cmd = PROCESS_CMD_STOP;
	sem_post(&module->next->new_cmd);

	module->proc_info->cmd = PROCESS_CMD_STOP;
	sem_post(&module->proc_info->sem_job);
    sem_wait(&module->proc_info->sem_result);

    return NULL;
}

void *processor_thread(void *data){
    module_t	*module = data;
    size_t		packetoffset;
    char		*packet;
    int cmd;

    module->id = process_start(module->argv);
    module->proc_info = (struct process_info *)(module->shmem + memory_get_processoffset(module->shmem)) + module->id;   

    while(1){
	    sem_wait(&module->new_cmd);
        cmd = module->cmd; //TODO: queue
        packetoffset = module->dataoffset;
        sem_post(&module->result);
	    if (cmd == PROCESS_CMD_STOP){
	        // previous process stop their job, finish pipeline
	        break;
	    }
        packet = module->shmem + packetoffset;
        fprintf(stdout, "[process_thread]: get packet %p, packet: %s\n", packet, packet);
        module->proc_info->cmd = cmd; 
        module->proc_info->cmd_offs = packetoffset;
	    sem_post(&module->proc_info->sem_job);
        sem_wait(&module->proc_info->sem_result);

	    if (module->proc_info->cmd_result != 0){
	        // previous process report an error, drop frame and continue
	        fprintf(stdout, "[process_thread]: processor status %d, drop packet %p\n",
		        module->proc_info->cmd_result, packet);
            //pushfree buffer
	        ring_buffer_push(module->data_ring, packetoffset); 
	        continue;
	    }
	    fprintf(stdout, "[process_thread]: processed packet %p, packet: %s\n", packet, packet);
	    sem_wait(&module->next->result);
	    module->next->cmd = PROCESS_CMD_JOB;
	    module->next->dataoffset = packetoffset;
	    sem_post(&module->next->new_cmd);
    }
    fprintf(stdout, "[process_thread]: stop\n");
	sem_wait(&module->next->result);
	module->next->cmd = PROCESS_CMD_STOP;
	sem_post(&module->next->new_cmd);
	
	module->proc_info->cmd = PROCESS_CMD_STOP;
	sem_post(&module->proc_info->sem_job);
    sem_wait(&module->proc_info->sem_result);
	
    return NULL;
}

void test_streamer(void *shmem, int *exit_flag) {
    
    int i;
    int modules_count = 2 + 1;
    module_t modules[modules_count];

    struct memory_map *map = (struct memory_map *)(shmem);

    struct process_info	*proc_info = (struct process_info *)(shmem + memory_get_processoffset(shmem));
    process_subsystem_init(proc_info, map->process_cnt);

    struct ring_buffer	*data_ring = ring_buffer_create(map->data_cnt, 0);
    for (i = 0; i < map->data_cnt; i++)
	    data_ring->buffer[i] = memory_get_dataoffset(shmem) + i * map->data_size;

    for (i = 0; i < modules_count; i++) {
        modules[i].shmem = shmem;
        sem_init(&modules[i].new_cmd, 0, 0);
        sem_init(&modules[i].result, 0, 1);
        modules[i].data_ring = data_ring;
        modules[i].exit_flag = exit_flag;
    }

    //TODO: list of threads (see process.c/.h) and read parameters from config (.json)
    //destination stream (it should free data in data queue), last
    char *proc0[] = { NULL };
    modules[0].argv = proc0;
    modules[0].thread_method = destination_thread;
    modules[0].next = NULL;
    //source stream (create and write data), first, any number of
    char *proc1[] = { "capturer", NULL };
    modules[1].argv = proc1;
    modules[1].thread_method = source_thread;
    modules[1].next = &modules[2];
    //filter stream (it used data as reader), any number of
    char *proc2[] = { "filter", NULL };
    modules[2].argv = proc2;
    modules[2].thread_method = processor_thread;
    modules[2].next = &modules[0];

    for (i = 0; i < modules_count; i++) {
        pthread_create(&modules[i].thread_id, NULL, modules[i].thread_method, &modules[i]); 
    }

    pthread_join(modules[2].thread_id, NULL);
    pthread_join(modules[1].thread_id, NULL);
    pthread_join(modules[0].thread_id, NULL);
    ring_buffer_destroy(data_ring);
    process_subsystem_destroy();
}

