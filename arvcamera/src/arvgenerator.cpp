#include <arv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "arvcamera/arvgenerator.h"

struct data_generator {
    int fd;
    int exit_flag;
    pthread_t thread_id;
    void *data;
    int curr;
};

static int cancel = 0;

static void set_cancel(int signal) {
    cancel = 1;
}

struct data_generator *generator_init(char *interface, char *serial){
    struct data_generator *generator = (struct data_generator *)malloc(sizeof(struct data_generator));
    generator->curr = -1;
    generator->exit_flag = 0;

    arv_debug_enable("all");
    //bin/arvcamera enp1s0:0 aaa or lo bbb
    ArvGvFakeCamera *gv_camera = arv_gv_fake_camera_new_full(interface, serial, NULL);
    generator->data = (void*)gv_camera;
    signal(SIGINT, set_cancel);
    if (arv_gv_fake_camera_is_running(gv_camera))
        while (!cancel) {
            generator->exit_flag = 0;
            sleep (1);
        }
    else
        fprintf(stderr, "Failed to start camera\n");
    g_object_unref(gv_camera);

    return generator;
}

void generator_destroy(struct data_generator *generator){
    free(generator->data);
    generator->exit_flag = 1;
    free(generator);
}



