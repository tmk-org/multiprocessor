#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//open
#include <sys/stat.h>
#include <fcntl.h>
//write/close/sleep
#include <unistd.h>

#include "capture/device.h"
#include "capture/camera.h"

void new_frame_action(void *buffer, unsigned int buffer_size, void *extended_data) {
    //this action is save-callback for buffer
    int frame_id = *(int*)extended_data;
    char fname[50];
    sprintf(fname, "../data/out_%d.raw", frame_id);
    fprintf(stdout, "[save_frame_callback]: out frame to %s as binary\n", fname);
    int out_fd;
    out_fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC, 0664);
    if (out_fd > 0) {
        write(out_fd, buffer, buffer_size);
        close(out_fd);
    }
}

void new_device_callback(void *internal_data, struct device *dev) {
    struct gige_camera *cam = gige_camera_init(dev, new_frame_action);
    (void)internal_data;
    fprintf(stdout, "I'm in the new device callback\n");
    fflush(stdout);
    device_print(dev);
    gige_camera_test(cam);
    gige_camera_destroy(cam);
    free(cam);
}

int main(int argc, char *argv[]) {

#if 1
    struct devices *devs = devices_discovery(NULL, new_device_callback);
#else
    struct devices *devs = prepare_devices_list();

    list_t *elem = list_first_elem(&devs->device_list);
    struct device *dev = list_entry(elem, struct device, entry);
    elem = elem->next;
    fprintf(stdout, "Found device\n");
    fflush(stdout);
    device_print(dev);
    if (new_device_callback) new_device_callback(NULL, dev);
    struct gige_camera *cam = gige_camera_init(dev, new_frame_action);
    gige_camera_test(cam);
    gige_camera_destroy(cam);
    free(dev);
#endif
    free(devs);

    return 0;
}

//#0  0x00007f2366f798b3 in __GI___select (nfds=4, readfds=0x7ffdcd86f430, 
//    writefds=0x0, exceptfds=0x0, timeout=0x7ffdcd86f420)
//#1  0x00007f23670affc4 in listen_packet_ack (fd=3)
//#2  0x00007f23670b01fb in write_register (fd=3, reg_addr=2560, value=2, 
//    packet_id=61443)
//#3  0x00007f23670b0356 in gvcp_init (fd=3, packet_id=61443, data_fd=4)
//#4  0x00007f23670b2571 in gige_camera_init (dev=0x555b682afa40, 
//    new_frame_action=0x555b66f35206 <new_frame_action>)




