#ifndef _CAMERA_H_
#define _CAMERA_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <pthread.h>
#include "capture/device.h"

enum gige_camera_state {
    STREAM_NOT_INITIALIZED = -1,
    STREAM_STOP = 0,
    STREAM_LIVE = 1,
    STREAM_PAUSE
};

struct gige_camera {
    struct device *dev;
    enum gige_camera_state state;

    int gvcp_fd;
    int gvsp_fd;
    //pthread_t control_thread_id; -- not neccessary: we sure that any camera will be started in the new source thread
    pthread_t data_thread_id;

    uint32_t last_gvcp_packet_id;

    uint32_t n_received_packets;
    uint32_t n_error_packets;
    uint32_t n_ignored_packets;

    uint64_t timestamp_tick_frequency;
    uint32_t packet_size;

    int extended_ids;
    uint32_t frame_id;
    uint32_t last_frame_id;

    void (*new_frame_action)(void *buffer, unsigned int buffer_size, void *extended_data);

};

//TODO: struct gige_camera *camera_init(char *sid_string);
struct gige_camera *gige_camera_init(struct device *dev, void (*new_frame_action)(void *, unsigned int, void *));
void gige_camera_destroy(struct gige_camera *cam);

int gige_camera_test(struct gige_camera *cam);
//int gige_camera_start(struct gige_camera *cam);
//int gige_camera_stop(struct gige_camera *cam);

#ifdef __cplusplus
}
#endif

#endif //_CAMERA_H_

