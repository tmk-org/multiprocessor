#include <stdio.h>
#include <string.h>

#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "shared_memory.h"
#include "shared_memory_map.h"

void *shared_memory_server_init(const char *shmpath, void *shmmap) {
    memory_map_t *map = (memory_map_t*)shmmap;
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

    memcpy(shmdata, map, sizeof(memory_map_t));

    close(fd);
    return shmdata;
}

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
    close(fd);
    return shmdata;
}

size_t shared_memory_size(void *shmem) {
    memory_map_t *map = (memory_map_t*)shmem;
    return memory_get_size(map);
}

void *shared_memory_map_create(int proc_cnt, size_t proc_size, int data_cnt, size_t data_size) {
    return memory_map_create(proc_cnt, proc_size, data_cnt, data_size);
}

int shared_memory_setenv(const char *shmpath, void *shmem) {
    memory_map_t *map = shmem;
    size_t shm_size = memory_get_size(map);
    char shm_size_str[40];

    if (setenv("SHMEM_DATA_NAME", shmpath, 1) != 0){
	    fprintf(stderr, "[shared_memory]: can't set SHMEM_DATA_NAME value for child process: errno=%d, %s\n",
		    errno, strerror(errno));
	    return -1;
    }

    snprintf(shm_size_str, sizeof(shm_size_str), "%zd", shm_size);
    if (setenv("SHMEM_DATA_SIZE", shm_size_str, 1) != 0){
	    fprintf(stderr, "[shared_memory]: can't set SHMEM_DATA_SIZE value for child process: errno=%d, %s\n",
		    errno, strerror(errno));
	    return -1;
    }

    return 0;
}

void shared_memory_unlink(const char *shmpath) {
    shm_unlink(shmpath);
}

void shared_memory_destroy(const char *shmpath, void *shmem) {
    munmap(shmem, memory_get_size(shmem));
    shm_unlink(shmpath);
}

