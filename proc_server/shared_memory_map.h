#ifndef _SHARED_MEMORY_MAP_H
#define _SHARED_MEMORY_MAP_H

#define MEMORY_ALIGN(size)	((((size) + 255) >> 8) << 8)
#define MEMORY_MAP_SIZE     MEMORY_ALIGN(sizeof(memory_map_t))

typedef struct memory_map {
    int	    max_process_cnt;
    size_t  max_process_size;
    int	    max_data_cnt;
    size_t  max_data_size;
} memory_map_t;

static inline void *memory_map_create(int proc_cnt, size_t proc_size, int data_cnt, size_t data_size) {
    memory_map_t *map = malloc(sizeof(memory_map_t));
    map->max_process_cnt = proc_cnt;
    map->max_process_size = proc_size;
    map->max_data_cnt = data_cnt;
    map->max_data_size = data_size;
    return (void*)map;
}

static inline size_t memory_get_size(memory_map_t *map){
    return MEMORY_MAP_SIZE +
	   MEMORY_ALIGN(map->max_process_size * map->max_process_cnt) +
	   MEMORY_ALIGN(map->max_data_size * map->max_data_cnt);
}

static inline size_t memory_get_mapoffset(void * shmem){
    (void)shmem;
    return 0;
}

static inline size_t memory_get_processoffset(void * shmem){
    (void)shmem;
    return MEMORY_MAP_SIZE;
}

static inline size_t memory_get_dataoffset(void * shmem){
    memory_map_t *map = shmem;
    return MEMORY_MAP_SIZE +
	   MEMORY_ALIGN(map->max_process_size * map->max_process_cnt);
}

#endif //_SHARED_MEMORY_MAP_H
