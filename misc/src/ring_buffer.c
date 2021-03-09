#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "misc/ring_buffer.h"

struct ring_buffer * ring_buffer_create(int max_cnt, int empty){
    size_t		size;
    struct ring_buffer	*ring;
    size = sizeof(struct ring_buffer) + max_cnt * sizeof(size_t);
    ring = malloc(size);
    if (ring == NULL){
	fprintf(stderr, "Error: can't allocate memory for ring buffer\n");
	return NULL;
    }

    memset(ring, 0, size);
    ring->max_cnt = max_cnt;
    if (!empty) ring->used = max_cnt;

    pthread_mutex_init(&ring->mutex, NULL);

    if (sem_init(&ring->sem, 0, empty ? 0 : max_cnt) == -1){
	fprintf(stderr, "Error: can't init ring buffer semaphore\n");
	free(ring);
	return NULL;
    }

    return ring;
}

void ring_buffer_destroy(struct ring_buffer *ring){
    pthread_mutex_destroy(&ring->mutex);
    sem_destroy(&ring->sem);
    free(ring);
}

size_t ring_buffer_pop(struct ring_buffer *ring){
    size_t offs;

    sem_wait(&ring->sem);
    pthread_mutex_lock(&ring->mutex);
    offs = ring->buffer[ring->start++];
    if (ring->start >= ring->max_cnt) ring->start = 0;
    ring->used--;
    pthread_mutex_unlock(&ring->mutex);
    return offs;
}

void ring_buffer_push(struct ring_buffer *ring, size_t shmem_offs){
    int pos;
    #warning "no overflow check, should not happen"
    pthread_mutex_lock(&ring->mutex);
    pos = ring->start + ring->used++;
    if (pos >= ring->max_cnt) pos -= ring->max_cnt;
    ring->buffer[pos] = shmem_offs;
    pthread_mutex_unlock(&ring->mutex);
    sem_post(&ring->sem);
}
