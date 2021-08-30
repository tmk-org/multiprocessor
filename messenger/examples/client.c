#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "messenger/messenger.h"

int main(int argc, char *argv[]){

    struct client *cli = client_create(NULL);
    tcp_client_process((void *)cli);
    client_destroy(cli);

    return 0;
}
