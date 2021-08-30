#include <stdio.h>
#include <stdlib.h>
#include <string.h>



int main(int argc, char *argv[]){

    if (argc != 2) {
        fprintf(stderr, "Usage: %s port\n", argv[0]);
        exit(EXIT_FAILURE);
    }


    return 0;
}
