#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include <sys/wait.h>

#include <pthread.h>
#include <execinfo.h>

#include "process_mngr.h"

#include "signal_reaction.h"

#define ARRAY_SIZE(a)	((int)(sizeof(a) / sizeof(a[0])))

static pthread_t            signal_thread_id;

void print_backtrace(FILE *f){
    int		fd;
    size_t	size;
    void	*array[200];

    fd = fileno(f);
    write(fd, "BACKTRACE ...\n", 14);
    size = backtrace(array, ARRAY_SIZE(array));
    backtrace_symbols_fd(array, size, fd);
    fsync(fd);
}

void sig_handler(int signum){
    if (signum == SIGCHLD){
	int	status;
	pid_t	pid;

	while((pid = waitpid((pid_t)(-1), &status, WNOHANG)) > 0){
	    fprintf(stderr, "SIGHANDLER(%d): process with pid=%d finished with status 0x%x\n",
	                    (int)getpid(), (int)pid, status);
	    fflush(stderr);
	}
	return;
    }
    fprintf(stderr, "SIGHANDLER(%d): signal %d received\n", (int)getpid(), signum);
    fflush(stderr);
    print_backtrace(stderr);
    exit(signum);
}

static void* signal_thread(void *data){
    siginfo_t           siginfo;
    sigset_t            signal_set;
    int		*exit_flag = data;
    int rc;

    (void)data;

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

        if (rc != -1){
	        switch (siginfo.si_signo){
		    case SIGCHLD:
		        process_cleanup_zombies();
		        break;
		    default:
                fprintf(stdout, "Exit signal received\n");
		        *exit_flag = 1;
		        break;
	        }
	    }
    }

    return NULL;
}

void signal_thread_init(int *exit_flag){
    if (pthread_create(&signal_thread_id, NULL,
                        signal_thread, exit_flag) != 0){
        fprintf(stderr, "Could not create event thread\n");
        exit(1);
    }
}

void signal_thread_destroy(void){
    pthread_cancel(signal_thread_id);
    pthread_join(signal_thread_id, NULL);
}

void set_signal_reactions(void){
    int			        signals[] = {SIGILL, SIGSEGV, SIGABRT, SIGFPE};
    int			        i;
    struct sigaction	action;

    sigemptyset(&action.sa_mask);
    action.sa_handler = sig_handler;
    action.sa_flags = SA_RESTART;

    for(i = 0; i < ARRAY_SIZE(signals); i++){
	    if (sigaction(signals[i], &action, NULL) < 0){
	        fprintf(stderr, "Can't set signal %d handler\n", signals[i]);
	        exit(EXIT_FAILURE);
	    }
    }

    sigemptyset(&action.sa_mask);
    sigaddset(&action.sa_mask, SIGCHLD); //child process managment
    sigaddset(&action.sa_mask, SIGINT);  //^C handler
    sigaddset(&action.sa_mask, SIGTERM); //kill without params
    if (pthread_sigmask(SIG_BLOCK, &action.sa_mask, NULL) != 0){
        fprintf(stderr, "Can't block SIGCHLD | SIGINT | SIGTERM signals.\n");
        exit(EXIT_FAILURE);
    }

}
