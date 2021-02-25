#ifndef _DCL_MEMORY_MAP_H
#define _DCL_MEMORY_MAP_H

#define MEMORY_ALIGN(size)	((((size) + 255) >> 8) << 8)
#define MEMORY_MAP_SIZE     MEMORY_ALIGN(sizeof(memory_map_t))

#if 0
//TODO: make config
typedef struct memory_map {
    int proc_num;
    size_t proc_offset;
    int fdesc_num;
    size_t fdesc_size;
    //buffer for memory allocate
    size_t fdesc_buffer_offset;
    size_t fdesc_busy_offset;
    size_t fdesc_free_offset; 
    int objdesc_num;
    size_t objdesc_size;
    //buffer for memory allocate
    size_t objdesc_buffer_offset;
    size_t objdesc_busy_offset;
    size_t objdesc_free_offset; 
    size_t buffer_size;   
} memory_map_t;
#else 
typedef struct memory_map {
    int	    max_process_cnt;
    size_t  max_process_size;
    int	    max_data_cnt;
    size_t  max_data_size;
} memory_map_t;

#endif

static inline size_t memory_get_size(memory_map_t *map){
    return MEMORY_MAP_SIZE +
	   MEMORY_ALIGN(map->max_process_size * map->max_process_cnt) +
	   MEMORY_ALIGN(map->max_data_size * map->max_data_cnt);
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

#endif //_DCL_MEMORY_MAP_H
