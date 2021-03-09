#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>

#include "shmem/shared_memory.h"
#include "shmem/shared_memory_map.h"

int main(int argc, char *argv[]){

    const char *shmpath = "/server_shmem";
    size_t		shmsize = 590080;

    void		*shmem;

    char data[256]; 

    const char *s = "Test_data_in_shared_memory";
    
    sprintf(data,"%zd%s", strlen(s), s);

    setlocale(LC_ALL, "");

    shmem = shared_memory_client_init(shmpath, shmsize);

    if (shmem == NULL) {
        fprintf(stderr, "[writer]: Can't init shared memory\n");
        exit(EXIT_FAILURE);
    }

    shared_memory_write_test_data(shmem, data, strlen(data));
    
    return 0;
}
