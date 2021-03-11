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

int main(int argc, char *argv[]){
    char		*shm_path = getenv("SHMEM_DATA_NAME");
    char		*shm_size_str = getenv("SHMEM_DATA_SIZE");
    char		*shm_id_str = getenv("SHMEM_PROCESS_ID");
    size_t		shm_size;
    int			shm_id;
    int			exit_flag = 0;
    char		*data;
    void		*shmem;
    struct process_info	*proc_info;

    if (shm_path == NULL){
	    fprintf(stderr, "Error/child: SHMEM_DATA_NAME is not set\n");
	    exit(EXIT_FAILURE);
    }
    if (shm_size_str == NULL){
	    fprintf(stderr, "Error child %s: SHMEM_DATA_SIZE is not set\n", argv[0]);
	    exit(EXIT_FAILURE);
    }
    if (shm_id_str == NULL){
	    fprintf(stderr, "Error child %s: SHMEM_DATA_ID is not set\n",argv[0]);
	    exit(EXIT_FAILURE);
    }

    sscanf(shm_size_str, "%zd", &shm_size);
    sscanf(shm_id_str, "%d", &shm_id);

    setlocale(LC_ALL, "");

    shmem = shared_memory_client_init(shm_path, shm_size);
    if (shmem == NULL)
	    exit(EXIT_FAILURE);

    proc_info = (struct process_info *)(shmem + memory_get_processoffset(shmem)) + shm_id;

    while(!exit_flag){
	    sem_wait(&proc_info->sem_job);
	    switch(proc_info->cmd){
	        case PROCESS_CMD_STOP:
		        fprintf(stdout, "[%s]: Child exit\n", argv[0]);
		        exit_flag = 1;
		        proc_info->cmd_result = 0;
		        break;
	        default:
		        data = shmem + proc_info->cmd_offs;
		        fprintf(stdout, "[%s]: Get: %s\n", argv[0], data);
                fflush(stdout);
		        //place for filter result
                //pass or drop
		        proc_info->cmd_result = (strchr(data, '*') == NULL);
		    break;
	    }
	    sem_post(&proc_info->sem_result);
    }
    return 0;
}
