#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "shmem/shared_memory_map.h"
#include "pmngr/process_mngr.h"
#include "misc/ring_buffer.h"

#include "streamer/pipeline.h"

struct pipeline{
    sem_t sem;
    void *shmem;
    struct process_info	*proc_info;
    struct ring_buffer	*data_ring;
    pthread_t   thread_id;
    int *exit_flag;
    struct pipeline	*prev;
};

void * pipeline_start_thread(void *data){
    struct pipeline	*p = data;
    struct process_info *proc_info = p->proc_info;

    size_t		packetoffset;
    char		*packet;

    while(!(*(p->exit_flag))){
	    packetoffset = ring_buffer_pop(p->data_ring);
	    packet = p->shmem + packetoffset;
	    printf("srv->thread_0: get frame %p\n", packet);

	    proc_info->cmd = PROCESS_CMD_JOB;
	    proc_info->cmd_offs = packetoffset;
	    sem_post(&proc_info->sem_job);
	    sem_wait(&p->sem);
    }
    printf("srv->thread_0: stop\n");
    proc_info->cmd = PROCESS_CMD_STOP;
    sem_post(&proc_info->sem_job);
    return NULL;
}

void * pipeline_middle_thread(void *data){
    struct pipeline	*p = data;
    struct pipeline	*prev = p->prev;
    struct process_info *proc_info = p->proc_info;
    struct process_info	*prev_info = prev->proc_info;
    size_t		packetoffset;
    char		*packet;

    while(1){
	    sem_wait(&prev_info->sem_result);
	    if (prev_info->cmd == PROCESS_CMD_STOP){
	        // previous process stop their job, finish pipeline
	        break;
	    }

	    packetoffset = prev_info->cmd_offs;
	    packet = p->shmem + packetoffset;

	    if (prev_info->cmd_result != 0){
	        // previous process report an error, drop frame and continue
	        sem_post(&prev->sem);
	        printf("srv->thread_1: capture error %d, drop packet %p\n",
		        prev_info->cmd_result, packet);
	        ring_buffer_push(p->data_ring, packetoffset);
	        continue;
	    }

	    sem_post(&prev->sem);

	    printf("srv->thread_1: capture packet %p, packet: %s\n", packet, packet);

	    proc_info->cmd = PROCESS_CMD_JOB;
	    proc_info->cmd_offs = packetoffset;
	    sem_post(&proc_info->sem_job);
	    sem_wait(&p->sem);
    }
    printf("srv->thread_1: stop\n");
    proc_info->cmd = PROCESS_CMD_STOP;
    sem_post(&proc_info->sem_job);
    return NULL;
}

void * pipeline_finish_thread(void *data){
    struct pipeline	*p = data;
    struct pipeline	*prev = p->prev;
    struct process_info	*prev_info = prev->proc_info;
    size_t		packetoffset;
    char		*packet;

    while(1){
	    sem_wait(&prev_info->sem_result);
	    if (prev_info->cmd == PROCESS_CMD_STOP){
	        // previous process stop their job, finish pipeline
	        break;
	    }

	    packetoffset = prev_info->cmd_offs;
	    packet = p->shmem + packetoffset;

	    if (prev_info->cmd_result != 0){
	        // previous process report an error, drop frame and continue
	        sem_post(&prev->sem);
	        printf("srv->thread_2: packet %p filtered, packet: %s\n", packet, packet);
	        ring_buffer_push(p->data_ring, packetoffset);
	        continue;
	    }

	    sem_post(&prev->sem);

	    printf("srv->thread_2: packet %p passed, packet: %s\n", packet, packet);

	    ring_buffer_push(p->data_ring, packetoffset);
    }
    printf("srv->thread_2: stop\n");
    return NULL;
}

void test_pipeline(void *shmem, int *p_exit_flag) {
    struct pipeline pipeline[3];
    int	i, id1, id2;
    char *proc1[] = {"../proc_client/capturer", NULL};
    char *proc2[] = {"../proc_client/filter", NULL};
    
    struct memory_map *map = (struct memory_map *)(shmem);

    struct process_info	*proc_info = (struct process_info *)(shmem + memory_get_processoffset(shmem));
    process_subsystem_init(proc_info, map->process_cnt);

    struct ring_buffer	*data_ring = ring_buffer_create(map->data_cnt, 0);
    for (i = 0; i < map->data_cnt; i++)
	    data_ring->buffer[i] = memory_get_dataoffset(shmem) + i * map->data_size;

    id1 = process_start(proc1);
    id2 = process_start(proc2);

    memset(pipeline, 0, sizeof(pipeline));
    for(i = 0; i < 3; i++){
	    sem_init(&pipeline[i].sem, 0, 0);
	    pipeline[i].shmem = shmem;
	    pipeline[i].data_ring = data_ring;
	    pipeline[i].exit_flag = p_exit_flag;
	    if (i != 0) pipeline[i].prev = &pipeline[i - 1];
    }

    pipeline[0].proc_info = &proc_info[id1];
    pipeline[1].proc_info = &proc_info[id2];
    pthread_create(&pipeline[0].thread_id, NULL, pipeline_start_thread, &pipeline[0]);
    pthread_create(&pipeline[1].thread_id, NULL, pipeline_middle_thread, &pipeline[1]);
    pthread_create(&pipeline[2].thread_id, NULL, pipeline_finish_thread, &pipeline[2]);

    pthread_join(pipeline[0].thread_id, NULL);
    pthread_join(pipeline[1].thread_id, NULL);
    pthread_join(pipeline[2].thread_id, NULL);
    ring_buffer_destroy(data_ring);
    process_subsystem_destroy();

}

