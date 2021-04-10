#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>

#include "pmngr/process_mngr.h"

static pthread_mutex_t process_mutex = PTHREAD_MUTEX_INITIALIZER;
static int process_cnt = 0;
static int process_cnt_max = 0;
static struct process_info	*process_info = NULL;


static int process_info_init(struct process_info *info) {
    memset(info, 0, sizeof(struct process_info));
    if (sem_init(&info->sem_job, 1, 0) == -1) {
	    fprintf(stderr, "[process_mngr]: can't init shared process semaphore\n");
	    return -1;
    }
    if (sem_init(&info->sem_result, 1, 0) == -1) {
	    sem_destroy(&info->sem_job);
	    fprintf(stderr, "[process_mngr]: can't init shared process semaphore\n");
	    return -1;
    }
    info->pid = (pid_t)(-1);
    return 0;
}

static void process_info_destroy(struct process_info *info){
    sem_destroy(&info->sem_job);
    sem_destroy(&info->sem_result);

    memset(info, 0, sizeof(struct process_info));
    info->pid = (pid_t)(-1);
}

int process_subsystem_init(struct process_info info[], int cnt) {
    int i, j;
    for (i = 0; i < cnt; i++) {
	    if (process_info_init(info + i) != 0) {
	        for (j = 0; j < i; j++) {
		        process_info_destroy(info + j);
		    }
	        return -1;
	    }
    }
    process_info = info;
    process_cnt_max = cnt;
    return 0;
}

void process_subsystem_destroy() {
    int i;

    for(i = 0; i < process_cnt_max; i++) {
	    process_info_destroy(process_info + i);
    }
    process_info = NULL;
    process_cnt_max = 0;
}

int process_start(char *argv[]) {
    int		id;
    pid_t	pid;
    char	id_str[10];

    pthread_mutex_lock(&process_mutex);
    if (process_cnt >= process_cnt_max) {
	    pthread_mutex_unlock(&process_mutex);
	    fprintf(stderr, "[process_mngr]: no more process allowed\n");
	    return -1;
    }
    id = process_cnt++;
    pthread_mutex_unlock(&process_mutex);

    if ((pid = fork()) == -1) {
	    fprintf(stderr, "[process_mngr]: starting new child failed on fork(): errno=%d, %s\n",
	    errno, strerror(errno));
	    return -1;
    }

    if (pid == 0) {
	    snprintf(id_str, sizeof(id_str), "%d", id);	    
	   
	    if (setenv(PROCESS_ID_ENV, id_str, 1) != 0) {
	        fprintf(stderr, "[process_mngr]: can't set %s value for child process: errno=%d, %s\n",
		        PROCESS_ID_ENV, errno, strerror(errno));
	        return -1;
	    }

        // close all parent fd except stdin(fd=0), stdout(fd=1), stderr(fd=2)
	    for (int fd = 3; fd < 1024; fd++) {
	        close(fd);
	    }

    #if 0
        int i = 0;
	    while (argv[i] != NULL ){
            fprintf(stdout, "argv[%d] = %s\n", i, argv[i]);
            i++;
	    }
	    fflush(stdout);
    #endif

	    execv(argv[0], argv);
	    fprintf(stderr, "[process_mngr]: can't exec child process %s: errno=%d, %s\n",
		    argv[0], errno, strerror(errno));
	    return -1;
    }
    process_info[id].pid = pid;
    snprintf(process_info[id].process_name, sizeof(process_info[id].process_name), "%s", argv[0]);
    return id;
}

void process_kill_all() {
    int i;

    //parallel stop
    for (i = 0; i < process_cnt; i++) {
	    process_info[i].cmd = PROCESS_CMD_STOP;
	    sem_post(&process_info->sem_job);
    }
    for (i = 0; i < process_cnt; i++) {
	    sem_wait(&process_info->sem_result);
    }
}

void process_cleanup() {
    pid_t pid;
    while (1) {
	    pid = waitpid((pid_t) (-1), NULL, WNOHANG);
	    if (pid <= 0) break;
    }
}

