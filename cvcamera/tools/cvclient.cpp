#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>

#include "cvcamera/cvcollector.h"

int main(int argc, char *argv[]){
    int data_id = 0;
    //TODO: array of collectors
    struct data_collector *collector = collector_init(argv[1], argv[2]);
    if (!collector) {
        fprintf(stdout, "[cvclient]: Can't init collector, may be camera isn't ready. Please, try again.\n");
        fflush(stdout);
        return 0;
    }
    while (!(collector->exit_flag)) {
        sem_wait(&(collector->data_ready));
        fprintf(stdout, "[cvclient]: New object %d ready\n", data_id++);
        fflush(stdout);
    }
    if (collector) collector_destroy(collector);
    return 0;
}



