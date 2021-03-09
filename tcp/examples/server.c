#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tcp/tcp_process.h"

static char *executeCommand(const char *cmd, int *exit_flag) {
    printf("[server]: TODO real execute command by api\n");
    if (strcasecmp(cmd, "STOP\n") == 0 || strcasecmp(cmd, "S\n") == 0) { 
        printf("[server]: STOP command recieved\n");
        *exit_flag = 1;
    }
    return strdup("OK\n");
}

int main(int argc, char *argv[]){
    
    if (argc != 2) {
	    fprintf(stderr, "Usage: %s port\n", argv[0]);
		exit(EXIT_FAILURE);
    }
    
#if 1
    tcp_server_process(argv[1], executeCommand);
#else
    tcp_server_process(argv[1], (execute_command_func_t)NULL);
#endif

    return 0;
}
