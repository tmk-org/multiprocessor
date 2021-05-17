#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <locale.h>

#include <string.h>

#include "pmngr/process_mngr.h"
#include "shmem/shared_memory.h"
#include "shmem/shared_memory_map.h"

#include "pipeline/pipeline.h"

void *pipeline_process_init(int *shm_id) {
    char *shm_path = getenv("SHMEM_DATA_NAME");
    char *shm_size_str = getenv("SHMEM_DATA_SIZE");
    char *shm_id_str = getenv("SHMEM_PROCESS_ID");
    size_t shm_size;

    if (shm_path == NULL){
    fprintf(stderr, "[client_interface_init, child]: SHMEM_DATA_NAME is not set\n");
    exit(EXIT_FAILURE);
    }
    if (shm_size_str == NULL){
    fprintf(stderr, "[client_interface_init, child]: SHMEM_DATA_SIZE is not set\n");
    exit(EXIT_FAILURE);
    }
    if (shm_id_str == NULL){
    fprintf(stderr, "[client_interface_init, child]: SHMEM_PROCESS_ID is not set\n");
    exit(EXIT_FAILURE);
    }

    sscanf(shm_size_str, "%zd", &shm_size);
    sscanf(shm_id_str, "%d", shm_id);

    setlocale(LC_ALL, "");

    void *shmem = shared_memory_client_init(shm_path, shm_size);
    if (shmem == NULL)
        exit(EXIT_FAILURE);

    struct process_info *proc_info = (struct process_info *)((char*)shmem +  memory_get_processoffset(shmem)) + (*shm_id);

    return shmem;
}

int pipeline_process(void (*pipeline_action) (void *data, int *status)) {
    int shm_id;
    void *shmem = pipeline_process_init(&shm_id);
    struct process_info *proc_info = (struct process_info *)((char*)shmem + memory_get_processoffset(shmem)) + shm_id;

    int exit_flag = 0;
    int status = -1;
    char *data;

    while(!exit_flag){
        sem_wait(&proc_info->sem_job);
        switch(proc_info->cmd){
            case PROCESS_CMD_STOP:
                fprintf(stdout, "[%s]: Child exit\n", proc_info->process_name);
                exit_flag = 1;
                proc_info->cmd_result = 0;
                break;
            default:
                data = (char*)((char*)shmem + proc_info->cmd_offs);
                fprintf(stdout, "[%s]: %s\n", proc_info->process_name, data);
                fflush(stdout);
                if (pipeline_action) {
                    pipeline_action((void*)(&data), &status);
                    fprintf(stdout, "[%s]: pipeline_action finished with status %d\n", proc_info->process_name, status);
                    fflush(stdout);
                    if (status == -1) {
                        fprintf(stdout, "[%s]: strange status, exiting\n", proc_info->process_name);
                        fflush(stdout);
                        exit_flag = 1;
                        status = 0;
                    }
                    proc_info->cmd_result = status;
                    break;
                }
                proc_info->cmd_result = 0;
                break;
        }
        sem_post(&proc_info->sem_result);
    }
    return 0;
}