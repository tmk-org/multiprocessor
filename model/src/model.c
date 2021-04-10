#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "shmem/shared_memory.h"
#include "shmem/shared_memory_map.h"
#include "pmngr/process_mngr.h"
#include "misc/ring_buffer.h"

#include "model/model.h"

//TODO: one configuration header
#include "config/config.h"
#include "config/module_description.h"

#define MAX_PATH_LENGTH 256 //max filename length

struct module{
    char **argv; //command string for start module
    void *shmem;
    int id;       //idx of process in the process table 
    struct process_info *proc_info;
    struct ring_buffer *data_ring;
    pthread_t thread_id;
    void * (*thread_method)(void*); //source, processor(filter) or destination
    sem_t new_cmd;
    int cmd;           //next action for module
    size_t dataoffset; //offset to data from start of shared memory for the next action
    sem_t result;
    int state;         //resulting state of last executed action 
    struct module *next; //for  sending commands to next module
    int *exit_flag;
};

struct model_desc{
    pthread_t thread_id;
    int exit_flag;
    char shmpath[MAX_PATH_LENGTH]; // '/name' and max name length 255
    void *shmem;
    //TODO: list of modules
    char *fname; //name of configuration file if exist (or NULL)
    int modules_count;
    struct module *modules;
};

void *destination_thread(void *arg){
    struct module *module = arg;
    size_t dataoffset;
    char *data;
    int cmd;

    module->id = -1;
#if 0 //no child process, only thread
    module->proc_info = (struct process_info *)(module->shmem + memory_get_processoffset(module->shmem)) + module->id;   
#endif
    while(1){
        sem_wait(&module->new_cmd);
        cmd = module->cmd; //TODO: queue
        dataoffset = module->dataoffset;
        sem_post(&module->result);
        if (cmd == PROCESS_CMD_STOP){
            // previous process stop their job, finish pipeline
            break;
        }
        data = (char*)(module->shmem + dataoffset);
        fprintf(stdout, "[destination_thread]: get data %p, data: %s\n", data, data);
#if 0 //no child process, only thread
        module->proc_info->cmd = cmd; 
        module->proc_info->cmd_offs = dataoffset;
        sem_post(&module->proc_info->sem_job);
        sem_wait(&module->proc_info->sem_result);

        if (module->proc_info->cmd_result != 0){
            // previous process report an error, drop data and continue
            printf("destination_thread: destination error %d, drop data %p\n", module->proc_info->cmd_result, data);
            //pushfree buffer
            ring_buffer_push(module->data_ring, dataoffset);
            continue;
        }
        printf("destination_thread: passed data %p, data: %s\n", data, data);
#endif
        ring_buffer_push(module->data_ring, dataoffset);
    }
    fprintf(stdout, "[destination_thread]: stop\n");
#if 0 //no child process, only thread
    module->proc_info->cmd = PROCESS_CMD_STOP;
    sem_post(&module->proc_info->sem_job);
    sem_wait(&module->proc_info->sem_result);
#endif
    return NULL;
}

void *source_thread(void *arg){
    struct module *module = arg;
    size_t dataoffset;
    char *data;
    int cmd;

    module->id = process_start(module->argv);
    module->proc_info = (struct process_info *)(module->shmem + memory_get_processoffset(module->shmem)) + module->id;   

    while(!(*(module->exit_flag))){
        cmd = module->cmd; //TODO: queue
        dataoffset = ring_buffer_pop(module->data_ring);
        data = module->shmem + dataoffset;
        fprintf(stdout, "[source_thread]: capture data %p, data %s\n", data, data);
        module->proc_info->cmd = cmd; 
        module->proc_info->cmd_offs = dataoffset;
        sem_post(&module->proc_info->sem_job);
        sem_wait(&module->proc_info->sem_result);
        fprintf(stdout, "[source_thread]: captured data %p, data: %s\n", data, data);
        sem_wait(&module->next->result);
        module->next->cmd = PROCESS_CMD_JOB;
        module->next->dataoffset = dataoffset;
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

void *processor_thread(void *arg){
    struct module *module = arg;
    size_t dataoffset;
    char *data;
    int cmd;

    module->id = process_start(module->argv);
    module->proc_info = (struct process_info *)(module->shmem + memory_get_processoffset(module->shmem)) + module->id;

    while(1){
        sem_wait(&module->new_cmd);
        cmd = module->cmd; //TODO: queue
        dataoffset = module->dataoffset;
        sem_post(&module->result);
        if (cmd == PROCESS_CMD_STOP){
            // previous process stop their job, finish pipeline
            break;
        }
        data = module->shmem + dataoffset;
        fprintf(stdout, "[process_thread]: get packet %p, packet: %s\n", data, data);
        module->proc_info->cmd = cmd;
        module->proc_info->cmd_offs = dataoffset;
        sem_post(&module->proc_info->sem_job);
        sem_wait(&module->proc_info->sem_result);

        if (module->proc_info->cmd_result != 0){
            // previous process report an error, drop frame and continue
            fprintf(stdout, "[process_thread]: processor status %d, drop data %p\n", module->proc_info->cmd_result, data);
            //pushfree buffer
            ring_buffer_push(module->data_ring, dataoffset);
            continue;
        }
        fprintf(stdout, "[process_thread]: processed data %p, data: %s\n", data, data);
        sem_wait(&module->next->result);
        module->next->cmd = PROCESS_CMD_JOB;
        module->next->dataoffset = dataoffset;
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

#if 1 //TODO: the place for this functions is in the module_description.c/.h
void out_description(struct module_description *description) {
    fprintf(stdout, "[%d] Command string: ", description->id);
    for (int i = 0; description->argv[i] != NULL; fprintf(stdout, "argv[%d]=%s ", i, description->argv[i]), i++);
    fprintf(stdout, "type: %d, next_id: %d\n", description->type, description->next);
    fflush(stdout);       
}

void out_configuration(int modules_count, struct module_description *descriptions) {
    for (int i = 0; i < modules_count; i++) {
        out_description(&descriptions[i]);
    }
}
#endif

int model_read_configuration(char *fname, struct module **p_modules) {
    struct module_description *descriptions;
    int modules_count = create_config(fname, &descriptions);
    out_configuration(modules_count, descriptions);
    //TODO: make normal name for this function 
    //mdesc->modules_count = model_description_read(fname, &descriptions);
    struct module *modules = (struct module *)malloc(modules_count * sizeof(struct module));
    int i, j;
    for (i = 0; i < modules_count; i++) {
        //module_id and it sequential number in config file mey be not equal
        //but next_id is corresponded with the module_id
        int idx = descriptions[i].id;
        //argv should be null-terminated
        modules[idx].argv = (char**)malloc(sizeof(descriptions[i].argv));
        for (j = 0; descriptions[i].argv[j] != NULL; j++) {
            modules[idx].argv[j] = strdup(descriptions[i].argv[j]);
        }
        modules[idx].argv[j] = NULL;
        if (descriptions[i].type == MT_FIRST) {
            modules[idx].thread_method = source_thread;
        }
        else if (descriptions[i].type == MT_LAST) {
            modules[idx].thread_method = destination_thread;
        }
        else if (descriptions[i].type == MT_MIDDLE){ //MT_MIDDLE
            modules[idx].thread_method = processor_thread;
        }
        modules[idx].next = (descriptions[i].next == -1) ? NULL : &modules[descriptions[i].next];
        //TODO: free coniguration structure (?)
        //free(descriptions[i]);
    }
    *p_modules = &(modules[0]);
    return modules_count;
}

int model_default_configuration(struct module **p_modules) {
    int modules_count = 2 + 1;
    struct module *modules = (struct module *)malloc(modules_count * sizeof(struct module));
    //TODO: list of threads (see process.c/.h) and read parameters from config (.json)
    //destination stream (it should free data in data queue), last
    modules[0].argv = (char **)malloc(1 * sizeof(char*));
    modules[0].argv[0] = NULL;
    modules[0].thread_method = destination_thread;
    modules[0].next = NULL;
    //source stream (create and write data), first, any number of
    modules[1].argv = (char **)malloc(2 * sizeof(char*));
    modules[1].argv[0] = strdup("mcapturer");
    modules[1].argv[1] = NULL;   
    modules[1].thread_method = source_thread;
    modules[1].next = &modules[2];
    //filter stream (it used data as reader), any number of
    modules[2].argv = (char **)malloc(2 * sizeof(char*));
    modules[2].argv[0] = strdup("mfilter");
    modules[2].argv[1] = NULL;  
    modules[2].thread_method = processor_thread;
    modules[2].next = &modules[0];
    *p_modules = &(modules[0]);
    return modules_count;
}

void* model_thread(void *arg) {
    struct model_desc *mdesc = (struct model_desc*)(arg);
    void *shmem = mdesc->shmem;
    int i;

    struct memory_map *map = (struct memory_map *)(shmem);

    struct process_info *proc_info = (struct process_info *)(shmem + memory_get_processoffset(shmem));
    process_subsystem_init(proc_info, map->process_cnt);

    struct ring_buffer *data_ring = ring_buffer_create(map->data_cnt, 0);
    for (i = 0; i < map->data_cnt; i++)
        data_ring->buffer[i] = memory_get_dataoffset(shmem) + i * map->data_size;

    int modules_count = mdesc->modules_count;
    for (i = 0; i < modules_count; i++) {
        struct module *m = &(mdesc->modules[i]);
        m->shmem = shmem;
        sem_init(&(m->new_cmd), 0, 0);
        sem_init(&(m->result), 0, 1);
        m->cmd = PROCESS_CMD_JOB;
        m->data_ring = data_ring;
        m->exit_flag = &mdesc->exit_flag;
    }

    //start modules threads
    for (i = 0;  i < modules_count; i++) {
        pthread_create(&((mdesc->modules[i]).thread_id), NULL, (mdesc->modules[i]).thread_method, &(mdesc->modules[i])); 
    }

    for (int i = modules_count - 1; i >=0; i--) {
        struct module *m = &(mdesc->modules[i]);
        pthread_join(m->thread_id, NULL);
        for (int j = 0; m->argv[j] != NULL; j++) {
            free(m->argv[j]);
        }
        free(m->argv);
    }
    ring_buffer_destroy(data_ring);
    process_subsystem_destroy();
    return NULL;
}

struct model_desc *model_init(char *fname) {
    struct model_desc *mdesc = malloc(sizeof(struct model_desc));
    if (!mdesc) {
        fprintf(stderr, "[model_init]: can't allocte model descriptor\n");
        fflush(stderr);
        return NULL;
    }
    strcpy(mdesc->shmpath, "/modelshm");
    mdesc->exit_flag = 0;
    void *map = shared_memory_map_create(3, PROCESS_INFO_SIZE, 512, 1024);
    shared_memory_unlink(mdesc->shmpath);
    void *shmem = shared_memory_server_init(mdesc->shmpath, map);
    free(map);

    if (shmem) {
        mdesc->shmem = shmem;
        shared_memory_setenv(mdesc->shmpath, shmem);
        if (fname) {
            mdesc->modules_count = model_read_configuration(fname, &(mdesc->modules));
        }
        else {
            mdesc->modules_count = model_default_configuration(&(mdesc->modules));
        }
        pthread_create(&mdesc->thread_id, NULL, model_thread, mdesc);
    }
    return mdesc;
}

void model_destroy(struct model_desc *mdesc) {
    mdesc->exit_flag = 1;
    pthread_join(mdesc->thread_id, NULL);
    shared_memory_destroy(mdesc->shmpath, mdesc->shmem);
    free(mdesc);
}
