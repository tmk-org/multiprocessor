#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>
#include <pthread.h>

#include "pmngr/signal_reaction.h"
#include "shmem/shared_memory.h"
#include "pmngr/process_mngr.h"

#include "tcp/tcp_process.h"
#include "model/command_server.h"
#include "model/model.h"
void parseReply(char *c){}; const char *getRequest() {return NULL;}; // it is for tcp, need to do something in tcp_process.c about it

void usage(char *pname) {
    fprintf(stderr, "Usage: %s /shmpath\n", pname);
}

struct manager_params {
    char shmpath[256];
    char port[8]; 
    int process_cnt;
    int data_cnt; 
    size_t data_size;
};

struct manager_params *parse_command_line(int argc, char *argv[]){
#if 0
    if (argc < 2) {
        usage(argv[0]);
	    exit(EXIT_FAILURE);
    }
#endif
    struct manager_params *mngr = malloc(sizeof(struct manager_params));
    strcpy(mngr->shmpath, "/modelshm");
    strcpy(mngr->port, "4444");
    mngr->process_cnt = 3;
    mngr->data_cnt = 512;
    mngr->data_size = 1024;
    return mngr;
}

//think about *exit_flag = -1/0/1 with -1 at start, 0 on init() and 1 on stop()
static char *executeCommand(const char *cmd, int *exit_flag) {
    //TODO: queue of commands, move command_server to libtcp
    fprintf(stdout, "[server]: TODO real execute command by api\n");
    fflush(stdout);
    if (strcasecmp(cmd, "STOP\n") == 0 || strcasecmp(cmd, "S\n") == 0) { 
        fprintf(stdout, "[server]: STOP command recieved\n");
        fflush(stdout);
        *exit_flag = 1;
    }
      // Create command and add into queue
      command_t *new_cmd = (command_t *)malloc(sizeof(command_t));
      memset(new_cmd, 0, sizeof(command_t));
      strcpy(new_cmd->full_command, cmd);
      //new_comm->fd = fd;  --- in the tcp_process
      fprintf(stdout, "Put to queue new command: '%s'\n", new_cmd->full_command);
      fflush(stdout);
      add_command(new_cmd);
    return strdup("OK\n");
}

int main(int argc, char *argv[]) {
    struct manager_params *mngr = parse_command_line(argc, argv);
    int	exit_flag = 0;
    //backtrace for exception
    setlocale(LC_ALL, "");
    set_signal_reactions();
    signal_thread_init(&exit_flag);
    if (command_server_init(&exit_flag) == 0) {
        tcp_server_process(mngr->port, executeCommand);
        command_server_destroy();
    }
    signal_thread_destroy();
    free(mngr);
    return 0;
}
