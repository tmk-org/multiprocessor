#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>
#include <pthread.h>

#include "pmngr/signal_reaction.h"
#include "shmem/shared_memory.h"

#include "pmngr/process_mngr.h"

#if defined PIPELINE
    #include "pipeline.h"
#elif defined STREAMER
    #include "streamer.h"
#endif

void usage(char *pname) {
    fprintf(stderr, "Usage: %s /shmpath\n", pname);
}

struct manager_params {
    char shmpath[256]; 
    int process_cnt;
    int data_cnt; 
    size_t data_size;
};

struct manager_params *parse_command_line(int argc, char *argv[]){
    if (argc < 2) {
        usage(argv[0]);
	    exit(EXIT_FAILURE);
    }
    struct manager_params *mngr = malloc(sizeof(struct manager_params));
    strcpy(mngr->shmpath, argv[1]);
    mngr->process_cnt = 3;
    mngr->data_cnt = 512;
    mngr->data_size = 1024;
    return mngr;
}

static int main_loop() {
  while (1) {
    //TODO: listen and execute commands
    sleep(1);
  }
  return 0;
}

int main(int argc, char *argv[]) {
    struct manager_params *mngr = parse_command_line(argc, argv);
    int	exit_flag = 0;
    //backtrace for exception
    setlocale(LC_ALL, "");
    set_signal_reactions();
    signal_thread_init(&exit_flag);
    //command_server_init();
    void *map = shared_memory_map_create(mngr->process_cnt, PROCESS_INFO_SIZE, mngr->data_cnt, mngr->data_size);
    shared_memory_unlink(mngr->shmpath);
    void *shmem = shared_memory_server_init(mngr->shmpath, map);
    free(map);
    if (shmem) shared_memory_setenv(mngr->shmpath, shmem);
    test_streamer(shmem, &exit_flag);
    //test_pipeline(shmem, &exit_flag);
    //main_loop();
    shared_memory_destroy(mngr->shmpath, shmem);
    //command_server_destroy();
    signal_thread_destroy();

    free(mngr);
    return 0;
}
