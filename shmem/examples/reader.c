#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>

#include "shmem/shared_memory.h"
#include "shmem/shared_memory_map.h"

int main(int argc, char *argv[]){

    const char *shmpath = "/server_shmem";
    size_t		size, shmsize = 590080;

    void		*shmem;

    char data[256], s[248];

    setlocale(LC_ALL, "");
    
    shmem = shared_memory_client_init(shmpath, shmsize);
    if (shmem == NULL) {
        fprintf(stderr, "[reader]: Can't init shared memory\n");
        exit(EXIT_FAILURE);
    }

    shared_memory_read_test_data(shmem, data, 256);
    sscanf(data, "%zd%s", &size, s);
    s[size]='\0';
    printf("Read %zd byte from shared: %s\n", size, s);

    return 0;
}
