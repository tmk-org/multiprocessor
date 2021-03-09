#ifndef _PROCESS_MNGR_H_
#define _PROCESS_MNGR_H_

#include <sys/types.h>
#include <semaphore.h>

#define PROCESS_MAX_CNT		10

#define PROCESS_CMD_STOP	0
#define PROCESS_CMD_JOB		1

#define PROCESS_DATA_MAX_LEN	2048

#define PROCESS_INFO_SIZE       sizeof(struct process_info)

#define PROCESS_ID_ENV      "SHMEM_PROCESS_ID"

struct process_info{
    sem_t		sem_job;
    sem_t		sem_result;
    pid_t		pid;
    int			process_state;
    char		process_name[80];
    int			cmd;
    int			cmd_result;
    union{
	    char		cmd_data[PROCESS_DATA_MAX_LEN];
	    size_t		cmd_offs;
    };
};

int  process_subsystem_init(struct process_info info[], int cnt);
void process_subsystem_destroy();
int  process_start(char *argv[]);
void process_kill_all();
void process_cleanup();

#endif /* _PROCESS_MNGR_H_ */

