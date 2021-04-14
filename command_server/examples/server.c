#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>
#include <pthread.h>

#include "tcp/tcp_process.h"
#include "command_server/command_server.h"

#if 0

void takeCommand(command_t *cmd, int *exit_flag, void (*callback)(command_t *)) {
    (void)callback;
#if 1
    //this function
    defaultExecuteCommand(cmd, exit_flag, defaultSendReply);
    //executeSimpleCommand(cmd, exit_flag);
#else
    //parse commands and execute it with defaultSendReply or your own callback
    //if more then one action, please, group it executors to this function
    executeAPICommand(cmd, exit_flag, defaultSendReply);
#endif
}
#endif

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

    struct command_server *cmdsrv = command_server_init(&exit_flag);
    if (cmdsrv != NULL) {
        tcp_server_process(argv[1], putCommand, 0);
        command_server_destroy(cmdsrv);
    }
    return 0;
}
