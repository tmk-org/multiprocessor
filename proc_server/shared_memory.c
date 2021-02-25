#include <stdio.h>
#include <string.h>
#include "memory_map.h"
#include "shared_memory.h"

void *shared_memory_server_init(const char *shmpath, memory_map_t *map) {
    int fd = shm_open(shmpath, O_CREAT|O_EXCL|O_RDWR, S_IRUSR|S_IWUSR);        
    if (fd == -1) {
        fprintf(stderr, "[shared_memory]: can't create shared memory\n");
        return NULL;
    }
    if (ftruncate(fd, memory_get_size(map)) == -1) {
        fprintf(stderr, "[shared_memory]: can't set shared memory size\n");
        shm_unlink(shmpath);
        close(fd);
        return NULL;
    }
    char *shmdata = mmap(NULL, memory_get_size(map),
                               PROT_READ | PROT_WRITE,
                               MAP_SHARED, fd, 0);
    if (shmdata == MAP_FAILED) {
        fprintf(stderr, "[shared_memory]: mmap error\n");
        shm_unlink(shmpath);
        close(fd);
        return NULL;
    }

#if 0
    memory_map_create(shmp, memory_map, MEMORY_MAP_SIZE);
#else
    memcpy(shmdata, map, MEMORY_MAP_SIZE);
#endif
    return shmdata;
}


//TODO: process_idx - whoami ???
void *shared_memory_client_init(const char *shmpath, size_t shmsize) {
    int fd = shm_open(shmpath, O_RDWR, 0);
    if (fd == -1) {
        fprintf(stderr, "[shared_memory]: can't open shared memory\n");
        return NULL;
    }
    char *shmdata = mmap(NULL, shmsize,
                               PROT_READ | PROT_WRITE,
                               MAP_SHARED, fd, 0);
    if (shmdata == MAP_FAILED) {
        fprintf(stderr, "[shared_memory]: mmap error\n");
        shm_unlink(shmpath);
        close(fd);
        return NULL;
    }
    return shmdata;
}

#if 0
int shared_memory_init(int is_child) {
    char *shmpath = "/testmem";
    if (is_child) {
        return shared_memory_client_init(shmpath);
    }
    return shared_memory_server_init(shmpath);       
}
#endif

void shared_memory_destroy(const char *shmpath) {
    shm_unlink(shmpath);
}

