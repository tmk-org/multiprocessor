#ifndef _SOURCE_H_
#define _SOURCE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>

struct source_data {
    int exit_flag;
    int gvcp_fd;
    int gvsp_fd;
    pthread_t control_thread_id;
    pthread_t data_thread_id;
    uint32_t packet_size;
    uint32_t n_received_packets;
    uint32_t frame_id;
    uint64_t timestamp_tick_frequency;
    
};

struct packet_data {
    int received;
    int64_t time_us;
};

struct frame_data {
    int is_started;
    int is_valid;
    char buffer[MAX_FRAME_SIZE];
    size_t real_size;

    struct gvsp_data_leader leader;

    int has_extended_ids;
    uint64_t frame_id;
    
    int n_packets;
    int error_packet_recieved;
    uint32_t last_valid_packet;
    struct packet_data *packet_data;
    
    uint64_t first_packet_time_us;
    uint64_t last_packt_time_us;

    //???
    int n_packets_resent;
    int resend_ratio_reached;
};

#ifdef __cplusplus
}
#endif

#endif //_SOURCE_H_

