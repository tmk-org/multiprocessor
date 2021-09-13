#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "messenger/messenger.h"

int main(int argc, char *argv[]){

    if (argc < 2) {
        fprintf(stderr, "Usage: %s srv_port [cli_host cli_port]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    //argv[0] and argv[1] are not about clients
    struct proxy *px = proxy_create((argc - 2) / 2 + 1);

    struct server *srv = server_create(argv[1], 10, NULL, NULL);
    proxy_add_server(px, srv);
    struct client *cli = client_create(NULL);
    proxy_add_client(px, cli);

    while (!px->exit_flag) {
        sleep(2);
    }

    proxy_destroy(px);


    return 0;
}
