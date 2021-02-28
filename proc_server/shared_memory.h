#ifndef _SHARED_MEM_H
#define _SHARED_MEM_H

void *shared_memory_map_create(int proc_cnt, size_t proc_size, int data_cnt, size_t data_size);

size_t shared_memory_size(void *shmem);

void *shared_memory_server_init(const char *shmpath, void *shmmap);
void *shared_memory_client_init(const char *shmpath, size_t shmsize);
void shared_memory_destroy(const char *shmpath, void *shmem);

void shared_memory_unlink(const char *shmpath);

int shared_memory_setenv(const char *shmpath, void *shmem);

#endif //_SHARED_MEM_H

