#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <pthread.h>

#include "signal_reaction.h"
#include "shared_memory.h"
#include "command_server.h"

#include "memory_map.h"
#include "process_mngr.h"

static int main_loop() {
  while (1) {
    //TODO: listen and execute commands
    sleep(1);
  }
  return 0;
}

#include "ring_buffer.h"

#if 0
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

void pipeline_process_test(void *shmem, memory_map_t *map, int *p_exit_flag) {
    struct pipeline pipeline[3];
    int	i, id1, id2;
    char *proc1[] = {"../proc_client/capturer", NULL};
    char *proc2[] = {"../proc_client/filter", NULL};

    struct process_info	*proc_info = (struct process_info *)(shmem + memory_get_processoffset(shmem));
    process_subsystem_init(proc_info, map->max_process_cnt);

    struct ring_buffer	*data_ring = ring_buffer_create(map->max_data_cnt, 0);
    for (i = 0; i < map->max_data_cnt; i++)
	    data_ring->buffer[i] = memory_get_dataoffset(shmem) + i * map->max_data_size;

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

#else

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
 
    char *proc[] = { NULL };

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

    char *proc[] = { "../proc_client/capturer", NULL };
 
    module->id = process_start(proc);
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
 
    char *proc[] = { "../proc_client/filter", NULL };

    module->id = process_start(proc);
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

void streamer_process_test(void *shmem, memory_map_t *map, int *exit_flag) {
    
    int i;
    int modules_count = 2 + 1;
    module_t modules[modules_count];

    struct process_info	*proc_info = (struct process_info *)(shmem + memory_get_processoffset(shmem));
    process_subsystem_init(proc_info, map->max_process_cnt);

    struct ring_buffer	*data_ring = ring_buffer_create(map->max_data_cnt, 0);
    for (i = 0; i < map->max_data_cnt; i++)
	    data_ring->buffer[i] = memory_get_dataoffset(shmem) + i * map->max_data_size;

    for (i = 0; i < modules_count; i++) {
        modules[i].shmem = shmem;
        sem_init(&modules[i].new_cmd, 0, 0);
        sem_init(&modules[i].result, 0, 1);
        modules[i].data_ring = data_ring;
        modules[i].exit_flag = exit_flag;
    }

    //TODO: list of threads (see process.c/.h) and read parameters from config (.json)
    //destination stream (it should free data in data queue), last
    modules[0].thread_method = destination_thread;
    modules[0].next = NULL;
    //source stream (create and write data), first, any number of
    modules[1].thread_method = source_thread;
    modules[1].next = &modules[2];
    //filter stream (it used data as reader), any number of
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

#endif

int main(int argc, char **argv) {
#if 0
    if (argc != 2) {
	    fprintf(stderr, "Usage: %s /shm-path\n", argv[0]);
	    exit(EXIT_FAILURE);
    }
    char *shmpath = argv[1];
#else
    const char *shmpath = "/testshm";
    shared_memory_destroy(shmpath);
#endif
    int	exit_flag = 0;
    //backtrace for exception
    setlocale(LC_ALL, "");
    //parse_command_line(argc, argv);
    set_signal_reactions();
    memory_map_t map = { 3, sizeof(struct process_info), 512, 1024 };
#if 1
    size_t		shm_size = memory_get_size(&map);
    char		shm_size_str[40];

    if (setenv("SHMEM_DATA_NAME", shmpath, 1) != 0){
	    fprintf(stderr, "ERROR: can't set SHMEM_DATA_NAME value for child process: errno=%d, %s\n",
		    errno, strerror(errno));
	    return -1;
    }

    snprintf(shm_size_str, sizeof(shm_size_str), "%zd", shm_size);
    if (setenv("SHMEM_DATA_SIZE", shm_size_str, 1) != 0){
	    fprintf(stderr, "ERROR: can't set SHMEM_DATA_SIZE value for child process: errno=%d, %s\n",
		    errno, strerror(errno));
	    return -1;
    }
#endif
    void *shmem = shared_memory_server_init(shmpath, &map);
    command_server_init();
    signal_thread_init(&exit_flag);
#if 0
    main_loop();
#else
    //pipeline_process_test(shmem, &map, &exit_flag);
    streamer_process_test(shmem, &map, &exit_flag);
#endif
    signal_thread_destroy();
    command_server_destroy();
    shared_memory_destroy(shmpath);
    return 0;
}
