#ifndef __RING_BUFFER_H__
#define __RING_BUFFER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>
#include <semaphore.h>

struct ring_buffer{
    pthread_mutex_t	mutex;
    sem_t		sem;
    int			max_cnt;
    int			used;
    int			start;
    size_t		buffer[0];
};

struct ring_buffer * ring_buffer_create(int max_cnt, int empty);
void                 ring_buffer_destroy(struct ring_buffer *ring);
size_t               ring_buffer_pop(struct ring_buffer *ring);
void                 ring_buffer_push(struct ring_buffer *ring, size_t shmem_offs);

#ifdef __cplusplus
}
#endif

#endif /* __SHARED_RING_BUFFER_H__ */
