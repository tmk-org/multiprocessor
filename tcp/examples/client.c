#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tcp/tcp_process.h"

int main(int argc, char *argv[]){

    if (argc != 3) {
        fprintf(stderr, "Usage: %s host port\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    tcp_client_process(argv[1], argv[2], 0, 0);

    return 0;
}
