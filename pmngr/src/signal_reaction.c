#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include <sys/wait.h>

#include <pthread.h>
#include <execinfo.h>

#include <signal.h>
#include <locale.h>

#include "pmngr/process_mngr.h"

#include "pmngr/signal_reaction.h"

#define MAX_SYMBOLS_CNT 150

#define _SIZE(x) ((int)(sizeof(x) / sizeof(x[0])))

static pthread_t signal_thread_id;

void print_backtrace(FILE *f) {
    int	fd;
    size_t size;
    void *buffer[MAX_SYMBOLS_CNT];

    fd = fileno(f);
    write(fd, "Backtrace...\n", 13);
    size = backtrace(buffer, _SIZE(buffer));
    backtrace_symbols_fd(buffer, size, fd);
    fsync(fd);
}

void signal_handler(int signum) {
    if (signum == SIGCHLD) {
	int	status;
	pid_t pid;

	while((pid = waitpid((pid_t)(-1), &status, WNOHANG)) > 0) {
	    fprintf(stderr, "HANDLER(%d): process with pid=%d finished with status 0x%x\n",
	                    (int)getpid(), (int)pid, status);
	    fflush(stderr);
	}
	return;
    }
    fprintf(stderr, "HANDLER(%d): signal %d received\n", (int)getpid(), signum);
    fflush(stderr);
    print_backtrace(stderr);
    exit(signum);
}

static void* signal_thread(void *data) {
    siginfo_t siginfo;
    sigset_t signal_set;
    int	*exit_flag = data;
    int rc;

    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

    // set signals to watch
    sigemptyset(&signal_set);
    sigaddset(&signal_set, SIGCHLD);
    sigaddset(&signal_set, SIGINT);
    sigaddset(&signal_set, SIGTERM);

    while(1){
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        rc = sigwaitinfo(&signal_set, &siginfo);
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

        if (rc != -1) {
	        switch (siginfo.si_signo) {
		    case SIGCHLD:
		        process_cleanup();
		        break;
		    default:
                fprintf(stdout, "[signal_reaction]: exit signal received\n");
		        *exit_flag = 1;
		        break;
	        }
	    }
    }

    return NULL;
}

void signal_thread_init(int *exit_flag) {
    if (pthread_create(&signal_thread_id, NULL, signal_thread, exit_flag) != 0) {
        fprintf(stderr, "[signal_reaction]: could not create event thread\n");
        exit(EXIT_FAILURE);
    }
}

void signal_thread_destroy(){
    pthread_cancel(signal_thread_id);
    pthread_join(signal_thread_id, NULL);
}

void set_signal_reactions() {
    int signals[] = { SIGILL, SIGSEGV, SIGABRT, SIGFPE };
    int	i, signals_cnt = _SIZE(signals);
    struct sigaction action;

    sigemptyset(&action.sa_mask);
    action.sa_handler = signal_handler;
    action.sa_flags = SA_RESTART;

    for(i = 0; i < signals_cnt; i++) {
	    if (sigaction(signals[i], &action, NULL) < 0) {
	        fprintf(stderr, "[signal_reaction]: can't set signal %d handler\n", signals[i]);
	        exit(EXIT_FAILURE);
	    }
    }

    sigemptyset(&action.sa_mask);
    sigaddset(&action.sa_mask, SIGCHLD); //child process managment
    sigaddset(&action.sa_mask, SIGINT);  //^C handler
    sigaddset(&action.sa_mask, SIGTERM); //kill without params
    if (pthread_sigmask(SIG_BLOCK, &action.sa_mask, NULL) != 0) {
        fprintf(stderr, "[signal_reaction]: can't block SIGCHLD | SIGINT | SIGTERM signals.\n");
        exit(EXIT_FAILURE);
    }

}
