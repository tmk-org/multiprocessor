#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "messenger/messenger.h"


int main(int argc, char *argv[]){

    if (argc != 2) {
        fprintf(stderr, "Usage: %s port\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    struct server *srv = server_create(argv[1], 10, NULL, NULL);
    tcp_server_process((void *)srv);
    server_destroy(srv);

    return 0;
}
