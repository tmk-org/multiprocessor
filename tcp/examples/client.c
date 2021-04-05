#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tcp/tcp_process.h"

const char *getRequest() {
    static char s[80];
    fprintf(stdout, "Message: ");
    fflush(stdout);
    memset(s, 0, sizeof(s));
    scanf("%78[^\n]%*c", s);
    strcat(s, "\n");
    return s;
}

void parseReply(char *s) {
    fprintf(stdout, "Reply: %s\n", s);
    fflush(stdout);
}

int main(int argc, char *argv[]){
    
    if (argc != 3) {
	    fprintf(stderr, "Usage: %s host port\n", argv[0]);
		exit(EXIT_FAILURE);
    }
    
#if defined NONBLOCK_INPUT
    tcp_client_with_select_process(argv[1], argv[2]);
#else
    tcp_client_process(argv[1], argv[2], parseReply, getRequest);
#endif

    return 0;
}
