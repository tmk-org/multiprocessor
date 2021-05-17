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
#include "command_server/command_server.h"
#include "pipeline/pipeline.h"


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
    strcpy(mngr->port, argv[1]);
    mngr->process_cnt = 3;
    mngr->data_cnt = 512;
    mngr->data_size = 1024;
    return mngr;
}

//think about *exit_flag = -1/0/1 with -1 at start, 0 on init() and 1 on stop()
char *putCommand(struct connection *connection, int *exit_flag) {
    char *request = connection->request;
    //TODO: queue of commands, move command_server to libtcp ???
    //this place is main stop for command server (it sets exit flag and then put stop command to queue)
    if (strcasecmp(request, "STOP\n") == 0 || strcasecmp(request, "S\n") == 0 ||
        strcasecmp(request, "STOP") == 0 || strcasecmp(request, "S") == 0 ||
        strcasecmp(request, "QUIT\n") == 0 || strcasecmp(request, "Q\n") == 0 ||
        strcasecmp(request, "QUIT") == 0 || strcasecmp(request, "Q") == 0 ||
        strcasecmp(request, "EXIT\n") == 0 || strcasecmp(request, "EXIT") == 0 ) {
        fprintf(stdout, "[command_server]: STOP/QUIT command recieved from %d\n", connection->fd);
        fflush(stdout);
        *exit_flag = 1;
    }
//------------------------------------------------------------------------------
// This is a place if we want do something before add command to queue
//------------------------------------------------------------------------------
    //Create command and add into queue
    push_new_command(connection->fd, connection->request);
    //TODO: answer the time when command was sent
    strcpy(connection->reply, "command succesfully sent");
    connection->reply_used = strlen(connection->reply);
    return connection->reply;
}

int main(int argc, char *argv[]) {
    int exit_flag = 0;

#if 0
    struct manager_params *mngr = parse_command_line(argc, argv);

    //backtrace for exception
    setlocale(LC_ALL, "");
    set_signal_reactions();
    signal_thread_init(&exit_flag);
#endif

    struct command_server *cmdsrv = command_server_init(&exit_flag);
    if (cmdsrv != NULL) {
        //tcp_server_process(mngr->port, putCommand, 0);
        tcp_server_process(argv[1], putCommand, 0);
        command_server_destroy(cmdsrv);
    }

#if 0
    signal_thread_destroy();

    free(mngr);
#endif

    return 0;
}


