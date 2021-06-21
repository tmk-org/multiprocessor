#include <arv.h>
#include <stdio.h>
#include <stdlib.h>

static int cancel = 0;

static void set_cancel(int signal) {
    cancel = 1;
}

int main(int argc, char *argv[]) {
    arv_debug_enable("all");
    //bin/arvcamera enp1s0:0 aaa
    ArvGvFakeCamera *gv_camera = arv_gv_fake_camera_new_full(argv[1], argv[2], NULL);
    signal(SIGINT, set_cancel);
    if (arv_gv_fake_camera_is_running(gv_camera))
        while (!cancel) {
            sleep (1);
        }
    else
        fprintf(stderr, "Failed to start camera\n");
    free(gv_camera);
    //g_object_unref(gv_camera);
    return 0;
}
