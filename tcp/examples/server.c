#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tcp/tcp_process.h"

static char *executeCommand(struct connection *connection, int *exit_flag) {
    default_server_reciever(connection, exit_flag);
    return connection->reply;
}

int main(int argc, char *argv[]){

    if (argc != 2) {
        fprintf(stderr, "Usage: %s port\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    tcp_server_process(argv[1], executeCommand, 0);

    return 0;
}
