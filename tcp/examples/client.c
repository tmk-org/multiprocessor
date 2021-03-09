#include <stdio.h>
#include <stdlib.h>

#include "tcp/tcp_process.h"

int main(int argc, char *argv[]){
    
    if (argc != 3) {
	    fprintf(stderr, "Usage: %s host port\n", argv[0]);
		exit(EXIT_FAILURE);
    }
    
#if defined NONBLOCK_INPUT
    tcp_client_with_select_process(argv[1], argv[2]);
#else
    tcp_client_process(argv[1], argv[2]);
#endif

    return 0;
}
