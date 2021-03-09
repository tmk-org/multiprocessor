#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "shmem/shared_memory.h"
#include "shmem/shared_memory_map.h"

static int main_loop() {
  while (1) {
    sleep(1);
  }
  return 0;
}

int main(int argc, char *argv[]) {
#if 0
    if (argc < 2) {
        fprintf(stderr, "Usage: %s /shmpath\n", argv[0]);
	    exit(EXIT_FAILURE);
    }
#endif
    
    const char *shmpath = "/server_shmem";
    char shmres[256];
    size_t shmsize;

    void *map = shared_memory_map_create(8, 65536/8, 512, 1024);
    printf("memsize = %zd\n", memory_get_size(map));

    //unlink if shmpath is in use
    shared_memory_unlink(shmpath);
    
    void *shmem = shared_memory_server_init(shmpath, map);
    free(map);
#if 0
    //This method is ok for translate enviroment variables from parent to child process
    if (shmem) shared_memory_setenv_default(shmpath, shmem);
    if (shared_memory_getenv_default(shmpath, &shmsize) == 0) {
        //TODO
    }
#endif
    main_loop();
    shared_memory_destroy(shmpath, shmem);
    
    return 0;
}

