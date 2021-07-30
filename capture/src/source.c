#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//network interface and socket
#include <sys/types.h>
#include <ifaddrs.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>


//open/write
#include <sys/stat.h>
#include <fcntl.h>

//usleep
#include <unistd.h>

#include "capture/gvcp.h"
#include "capture/gvsp.h"
#include "capture/source.h"

void* source_data_thread_udp(void *arg) {
    int extended_ids = 0;
    uint32_t packet_id;
    uint64_t frame_id;
    uint64_t last_frame_id = 0;
    uint32_t i; //counter for packets
    struct source_data *src = (struct source_data *)arg;
    int fd = src->fd;
    int n_received_packets = 0;
    int n_error_packets = 0;
    int n_ignored_packets = 0;
    struct sockaddr device_addr;
    socklen_t device_addr_len = sizeof(struct sockaddr);
    fd_set readfd;
    int rc = 0;
    ssize_t bytes = 0;
    struct timeval tv;

    struct frame_data *frame = (struct frame_data*)malloc(sizeof(struct frame_data));
    frame->is_started = 0;

    //fprintf(stdout, "[0] Hello from source_data_thread_udp, fd = %d\n", fd);
    //fflush(stdout);

    uint32_t packet_size = src->packet_size - IP_HEADER_SIZE - UDP_HEADER_SIZE;
    struct gvsp_packet *packet = (struct gvsp_packet*)malloc(packet_size);

    while (!src->exit_flag) {
        //fprintf(stdout, "[source_data_thread_udp]: [1] Hello from source_data_thread_udp\n");
        //fflush(stdout);
        FD_ZERO(&readfd);
        FD_SET(fd, &readfd);
        tv.tv_sec = 0;
        tv.tv_usec = 500000;
        rc = select(fd + 1, &readfd, NULL, NULL, &tv);
        if (rc > 0) {
            if (FD_ISSET(fd, &readfd)) {
                //fprintf(stdout, "[source_data_thread_udp]: [2] Hello from source_data_thread_udp before read\n");
                //fflush(stdout);
                bytes = recvfrom(fd, packet, packet_size, 0, &device_addr, &device_addr_len);
                //fprintf(stdout, "[source_data_thread_udp]: bytes = %ld, packet_size = %d\n", bytes, packet_size);
                //fflush(stdout);
                if (bytes < 0) {
                    fprintf(stdout, "[source_data_thread_udp]: no data on inteface\n");
                    fflush(stdout);
                    free(packet);
                    return NULL;
                }
                //work with packet
                n_received_packets++;
                extended_ids = gvsp_packet_has_extended_ids(packet);
                frame_id = gvsp_packet_get_frame_id(packet);
                packet_id = gvsp_packet_get_packet_id(packet);
                //fprintf(stdout, "[data_thread_udp]: total = %d ", n_received_packets);
                //fprintf(stdout, "packet_id = 0x%hx(%d), frame_id = 0x%lx(%lu)\n", packet_id, packet_id, frame_id, frame_id);
                //fflush(stdout);

                //find frame data
                //now is primitive:  check that frame_id > last_frame_id
                if (frame_id > last_frame_id) {
                    if (frame->is_started) {
                        char fname[50];
                        sprintf(fname, "data/out_%ld.raw", last_frame_id);
                        fprintf(stdout, "[source_data_thread_udp]: out frame to %s as binary\n", fname);
                        int out_fd;
                        out_fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC, 0664);
                        if (out_fd > 0) {
                            write(out_fd, frame->buffer, frame->real_size);
                            close(out_fd);
                        }
                    }
                    fprintf(stdout, "[source_data_thread_udp]: new frame %lu found\n", frame_id);
                    fflush(stdout);
                    frame->real_size = 0;
                    frame->frame_id = frame_id;
                    frame->last_valid_packet = -1;
                    frame->has_extended_ids = extended_ids;
                    frame->is_started = 1;
                    frame->n_packets = 0;
                    frame->error_packets_recieved = 0;
                    last_frame_id = frame->frame_id;
                }

                if (frame->is_started) {
                    enum gvsp_packet_type packet_type = gvsp_packet_get_packet_type(packet);
                    if (gvsp_packet_type_is_error(packet_type)) {
                        //TODO:
                        fprintf(stdout, "[source_data_thread_udp]: error packet recieved\n");
                        fflush(stdout);
                        n_error_packets++;
                        frame->error_packet_recieved++;
                    }
                    //TODO: think about necessary of this case
                    else if (packet_id < frame->n_packets && frame->packet_data[packet_id].received) {
                        fprintf(stdout, "[source_data_thread_udp]: very strange -- duplicate packet recieved\n");
                        fflush(stdout);
                    }
                    else {
                        //fprintf(stdout, "[source_data_thread_udp]: normal packet recieved\n");
                        //fflush(stdout);
                        enum gvsp_content_type content_type = gvsp_packet_get_content_type(packet);;
                        if (packet_id < frame->n_packets) {
                            frame->packet_data[packet_id].received = 1;
                        }
                        //tracking packets continuious
                        for (i = frame->last_valid_packet + 1; i < frame->n_packets; i++)
                            if (!frame->packet_data[i].received) break;
                        frame->last_valid_packet = i - 1;

                        switch (content_type) {
                            case GVSP_CONTENT_TYPE_DATA_LEADER: {
                                //process data leader
                                struct gvsp_data_leader *leader = &frame->leader;
                                leader->payload_type = gvsp_packet_get_payload_type(packet);
                                if (leader->payload_type == GVSP_PAYLOAD_TYPE_IMAGE) {
                                    fprintf(stdout, "[source_data_thread_udp]: image data (timestamp = %lu): width = %u, height = %u, offset_x = %u, offset_y = %u, pixel_format = 0x%x\n", gvsp_packet_get_timestamp(packet, src->timestamp_tick_frequency), 
                                            gvsp_packet_get_width(packet),
                                            gvsp_packet_get_height(packet),
                                            gvsp_packet_get_x_offset(packet),
                                            gvsp_packet_get_y_offset(packet),
                                            gvsp_packet_get_pixel_format(packet));
                                    fflush(stdout);
                                }
                                else {
                                    fprintf(stdout, "[source_data_thread_udp]: strange payload type\n");
                                    fflush(stdout);
                                }
                                break;
                            }
                            case GVSP_CONTENT_TYPE_DATA_BLOCK: {
#if 0
                                size_t block_size;
                                ptrdiff_t block_offset;
                                ptrdiff_t block_end;
                                size_t block_header = sizeof(struct gvsp_packet) + sizeof(struct gvsp_header);
                                block_size = gvsp_packet_get_data_size(packet, packet_size);
                                block_offset = (packet_id - 1) * (packet_size - block_header);
                                block_end = block_size + block_offset;
                                //TODO: block_end vs buffer_size
                                memcpy(frame->buffer + frame->real_size, gvsp_packet_get_data(packet), block_size);
                                frame->real_size += block_size;
#else
                                //process data block -- update this place without copy memory
                                memcpy(frame->buffer + frame->real_size, gvsp_packet_get_data(packet), gvsp_packet_get_data_size(packet, packet_size));
                                frame->real_size += gvsp_packet_get_data_size(packet, packet_size);
#endif
                                //fprintf(stdout, "frame->real_size = %zd\n", frame->real_size);
                                //fflush(stdout);
                                break;
                            }
                            case GVSP_CONTENT_TYPE_DATA_TRAILER:
                                if (frame->error_packet_recieved > 0) {
                                    fprintf(stdout, "%d error packets recived, ", );
                                    fflush(stdout);
                                }
                                else {
                                    fprintf(stdout, "no error packets recieved, ");
                                    fflush(stdout);
                                }
                                fprintf(stdout, "frame->real_size = %zd\n", frame->real_size);
                                fflush(stdout);
                                //process data trailer
                                break;
                            default:
                                n_ignored_packets++;
                        }

                    //TODO: check missing packets

                    }
                }
                else {
                    fprintf(stdout, "ignore packet\n");
                    fflush(stdout);
                    n_ignored_packets++;
                }
            }
        }
        else {
#if 0
            static int x = 0;
            if (x++ % 1000 == 0) {
                fprintf(stdout, "no data\n");
                fflush(stdout);
                x /= 1000;
            }
#else
            //fprintf(stdout, "[source_data_thread_udp]: no data on inteface\n");
            //fflush(stdout);
#endif
            usleep(100);
            continue;
        }
    }
    free(packet);
    free(frame);
    fprintf(stdout, "[source_data_thread_udp]: stop\n");
    fflush(stdout);
    return NULL;
}

/*

//------------ Initialize camera registers for acquisition and start source -------------------------//
    //Initialize source
    struct source_data *src = (struct source_data *)malloc(sizeof(struct source_data));
    src->exit_flag = 0;
    src->fd = data_fd;

    //Read a number of stream channels register from the camera socket
    read_register(fd, GV_REGISTER_N_STREAM_CHANNELS_OFFSET, &value, packet_id);
    read_register(fd, GV_GVCP_CAPABILITY_OFFSET, &value, next_packet_id(packet_id));

    //Read camera parameters
    uint32_t timestamp_tick_frequency_high;
    uint32_t timestamp_tick_frequency_low;

    read_register(fd, GV_TIMESTAMP_TICK_FREQUENCY_HIGH_OFFSET, &timestamp_tick_frequency_high, next_packet_id(packet_id));
    read_register(fd, GV_TIMESTAMP_TICK_FREQUENCY_LOW_OFFSET, &timestamp_tick_frequency_low, next_packet_id(packet_id));
    src->timestamp_tick_frequency = ((uint64_t) timestamp_tick_frequency_high << 32) | timestamp_tick_frequency_low;

    //Control Channel Privelege on
    write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x02, next_packet_id(packet_id));
    //Write Stream Channel 0 Destination address, port and packet_size
    write_register(fd, GV_STREAM_CHANNEL_0_IP_ADDRESS_OFFSET, ntohl(addr.sin_addr.s_addr), next_packet_id(packet_id));
    write_register(fd, GV_STREAM_CHANNEL_0_PORT_OFFSET, port, next_packet_id(packet_id));
    read_register(fd, GV_STREAM_CHANNEL_0_PACKET_SIZE_OFFSET, &src->packet_size, next_packet_id(packet_id));

    //Start aquistion
    write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x02, next_packet_id(packet_id));
#if 0
    write_register(fd, 0x124, 0x01, next_packet_id(packet_id));
#else

    write_register(fd, 0xF0F00A08, 0x00000000, next_packet_id(packet_id));
    read_register(fd,  0xF0F00A0C, &value, next_packet_id(packet_id));
    write_register(fd, 0xF0F00A0C, value, next_packet_id(packet_id));
    write_register(fd, 0xF0F00A08, 0x00000000, next_packet_id(packet_id));
    read_register(fd,  0xF0F00960, &value, next_packet_id(packet_id));
    read_register(fd,  0xF0F00964, &value, next_packet_id(packet_id));
    read_register(fd,  0xF0F00530, &value, next_packet_id(packet_id));
    read_register(fd,  0xF0F00830, &value, next_packet_id(packet_id));
    write_register(fd, 0xF0F00830, 0x80000000, next_packet_id(packet_id));
    read_register(fd,  0xF0F0083C, &value, next_packet_id(packet_id));
    write_register(fd, 0xF0F0083C, value, next_packet_id(packet_id));
    read_register(fd,  0xF0F0053C, &value, next_packet_id(packet_id));
    read_register(fd,  0xF0F0083C, &value, next_packet_id(packet_id));
    write_register(fd, 0xF0F0083C, 0xC2000610, next_packet_id(packet_id));
    write_register(fd, 0xF0F00968, 0x41200000, next_packet_id(packet_id)); //?Physical link configuration
    read_register(fd,  0xF0F05410, &value, next_packet_id(packet_id));
    read_register(fd,  0xF0F04050, &value, next_packet_id(packet_id));
    read_register(fd,  0xF0F04054, &value, next_packet_id(packet_id));

    write_register(fd, GV_STREAM_CHANNEL_0_PORT_OFFSET, port, next_packet_id(packet_id));
    read_register(fd,  GV_STREAM_CHANNEL_SOURCE_PORT_OFFSET, &value, next_packet_id(packet_id));
    write_register(fd, 0xF0F04030, 0x80000000, next_packet_id(packet_id));
#endif
    fprintf(stdout, "Acquisition started\n");
    fflush(stdout);

    //Start source
    pthread_create(&thread_id, NULL, source_data_thread_udp, src);
    fprintf(stdout, "Data thread created on fd = %d\n", src->fd);
    fflush(stdout);

    int cnt = 0;
    while(cnt < 2) {
        read_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, &value, next_packet_id(packet_id));
        if (value != 0x02) write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x02, next_packet_id(packet_id));
        write_register(fd, 0xF0F04030, 0x80000000, next_packet_id(packet_id));
        read_register(fd,  0x00000d1c, &value, next_packet_id(packet_id));
        sleep(1);
        cnt++;
    }

    //Stop acquisition
    read_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, &value, next_packet_id(packet_id));
    if (value != 0x02) write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x02, next_packet_id(packet_id));
#if 0
    write_register(fd, 0x124, 0x00, next_packet_id(packet_id));
#else
    read_register(fd,  0xF0F00614, &value, next_packet_id(packet_id));
    write_register(fd, 0xF0F00614, 0x00000000, next_packet_id(packet_id));
#endif
    fprintf(stdout, "Acquisition stopped\n");
    fflush(stdout);

    //Stop source
    src->exit_flag = 1;
    sleep(1);
    pthread_join(thread_id, NULL);

    //Control Channel Privelege off
    write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x00, next_packet_id(packet_id));

    free(src);
    return fd;
} */


void aravis-fake-start(int fd, uint16_t packet_id) {
    write_register(fd, 0x00000124, 0x01, packet_id);
}

void aravis-fake-stop(int fd, uint16_t packet_id) {
    write_register(fd, 0x00000124, 0x00, packet_id);
}

void flir-color-start(int fd, uint16_t packet_id) {
    write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x02, next_packet_id(packet_id));

    write_register(fd, 0xF0F00A08, 0x00000000, next_packet_id(packet_id));
    read_register(fd,  0xF0F00A0C, &value, next_packet_id(packet_id));
    write_register(fd, 0xF0F00A0C, value, next_packet_id(packet_id));
    write_register(fd, 0xF0F00A08, 0x00000000, next_packet_id(packet_id));
    read_register(fd,  0xF0F00960, &value, next_packet_id(packet_id));
    read_register(fd,  0xF0F00964, &value, next_packet_id(packet_id));
    read_register(fd,  0xF0F00530, &value, next_packet_id(packet_id));
    read_register(fd,  0xF0F00830, &value, next_packet_id(packet_id));
    write_register(fd, 0xF0F00830, 0x80000000, next_packet_id(packet_id));
    read_register(fd,  0xF0F0083C, &value, next_packet_id(packet_id));
    write_register(fd, 0xF0F0083C, value, next_packet_id(packet_id));
    read_register(fd,  0xF0F0053C, &value, next_packet_id(packet_id));
    read_register(fd,  0xF0F0083C, &value, next_packet_id(packet_id));
    write_register(fd, 0xF0F0083C, 0xC2000610, next_packet_id(packet_id));
    write_register(fd, 0xF0F00968, 0x41200000, next_packet_id(packet_id)); //?Physical link configuration
    read_register(fd,  0xF0F05410, &value, next_packet_id(packet_id));
    read_register(fd,  0xF0F04050, &value, next_packet_id(packet_id));
    read_register(fd,  0xF0F04054, &value, next_packet_id(packet_id));

    write_register(fd, GV_STREAM_CHANNEL_0_PORT_OFFSET, port, next_packet_id(packet_id));
    read_register(fd,  GV_STREAM_CHANNEL_SOURCE_PORT_OFFSET, &value, next_packet_id(packet_id));
    write_register(fd, 0xF0F04030, 0x80000000, next_packet_id(packet_id));
}

void flir-color-stop(int fd, uint16_t packet_id) {
    read_register(fd,  0xF0F00614, &value, next_packet_id(packet_id));
    write_register(fd, 0xF0F00614, 0x00000000, next_packet_id(packet_id));
}

void flir-color-watchdog(int fd, uint16_t packet_id) {
    write_register(fd, 0xF0F04030, 0x80000000, next_packet_id(packet_id));
    read_register(fd,  GV_STREAM_CHANNEL_SOURCE_PORT_OFFSET, &value, next_packet_id(packet_id));
}


void *data_thread_udp(void *arg) {
    struct source_data *src = (struct source_data *)arg;
    while (!src->exit_flag) {
        
    }
    fprintf(stdout, "Acquisition stopped\n");
    fflush(stdout);
}

void *control_thread_udp(void *arg) {
    struct source_data *src = (struct source_data *)arg;

    fprintf(stdout, "Command thread started\n");
    fflush(stdout);

    init_camera(src->gvcp_fd);

    //Start source
    pthread_create(&control_thread_id, NULL, source_data_thread_udp, src);
    fprintf(stdout, "Data thread created on fd = %d\n", src->gvsp_fd);
    fflush(stdout);

    int cnt = 0;
    while (!src->exit_flag && cnt < 2) {
        read_register(src->gvcp_fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, &value, next_packet_id(packet_id));
        if (value != 0x02) write_register(src->gvcp_fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x02, next_packet_id(packet_id));
        //watchdog callback
        if (src->special_watchdog != NULL) src->special_watchdog(src, next_packet_id(packet_id));
        read_register(src->gvcp_fd, GV_STREAM_CHANNEL_SOURCE_PORT_OFFSET, &value, next_packet_id(packet_id)); //???
        sleep(1);
        cnt++;
    }
}

struct source_data *source_init(int gvcp_fd, int gvsp_fd) {
    //Initialize source
    struct source_data *src = (struct source_data *)malloc(sizeof(struct source_data));
    src->exit_flag = 0;
    src->gvcp_fd = gvcp_fd;
    src->gvsp_fd = gvsp_fd;

    //Start source
    pthread_create(&control_thread_id, NULL, control_thread_udp, src);

    fprintf(stdout, "Command thread created for gvcp_fd = %d and gvsp_fd = %d\n", src->gvcp_fd, src->gvcp_fd);
    fflush(stdout);

    return src;
}

void source_destroy(struct source_data *src) {
    src->exit_flag = 1;
    sleep(1);
    pthread_join(data_thread_id, NULL);
    sleep(1);
    pthread_join(control_thread_id, NULL);
    free(src);
}


    //Start aquistion
    write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x02, next_packet_id(packet_id));
#if 0
    write_register(fd, 0x124, 0x01, next_packet_id(packet_id));
#else

    write_register(fd, 0xF0F00A08, 0x00000000, next_packet_id(packet_id));
    read_register(fd,  0xF0F00A0C, &value, next_packet_id(packet_id));
    write_register(fd, 0xF0F00A0C, value, next_packet_id(packet_id));
    write_register(fd, 0xF0F00A08, 0x00000000, next_packet_id(packet_id));
    read_register(fd,  0xF0F00960, &value, next_packet_id(packet_id));
    read_register(fd,  0xF0F00964, &value, next_packet_id(packet_id));
    read_register(fd,  0xF0F00530, &value, next_packet_id(packet_id));
    read_register(fd,  0xF0F00830, &value, next_packet_id(packet_id));
    write_register(fd, 0xF0F00830, 0x80000000, next_packet_id(packet_id));
    read_register(fd,  0xF0F0083C, &value, next_packet_id(packet_id));
    write_register(fd, 0xF0F0083C, value, next_packet_id(packet_id));
    read_register(fd,  0xF0F0053C, &value, next_packet_id(packet_id));
    read_register(fd,  0xF0F0083C, &value, next_packet_id(packet_id));
    write_register(fd, 0xF0F0083C, 0xC2000610, next_packet_id(packet_id));
    write_register(fd, 0xF0F00968, 0x41200000, next_packet_id(packet_id)); //?Physical link configuration
    read_register(fd,  0xF0F05410, &value, next_packet_id(packet_id));
    read_register(fd,  0xF0F04050, &value, next_packet_id(packet_id));
    read_register(fd,  0xF0F04054, &value, next_packet_id(packet_id));

    write_register(fd, GV_STREAM_CHANNEL_0_PORT_OFFSET, port, next_packet_id(packet_id));
    read_register(fd,  GV_STREAM_CHANNEL_SOURCE_PORT_OFFSET, &value, next_packet_id(packet_id));
    write_register(fd, 0xF0F04030, 0x80000000, next_packet_id(packet_id));
#endif
    fprintf(stdout, "Acquisition started\n");
    fflush(stdout);

    //Start source
    pthread_create(&thread_id, NULL, source_data_thread_udp, src);
    fprintf(stdout, "Data thread created on fd = %d\n", src->fd);
    fflush(stdout);

    int cnt = 0;
    while(cnt < 2) {
        read_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, &value, next_packet_id(packet_id));
        if (value != 0x02) write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x02, next_packet_id(packet_id));
        write_register(fd, 0xF0F04030, 0x80000000, next_packet_id(packet_id));
        read_register(fd,  0x00000d1c, &value, next_packet_id(packet_id));
        sleep(1);
        cnt++;
    }

    //Stop acquisition
    read_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, &value, next_packet_id(packet_id));
    if (value != 0x02) write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x02, next_packet_id(packet_id));
#if 0
    write_register(fd, 0x124, 0x00, next_packet_id(packet_id));
#else
    read_register(fd,  0xF0F00614, &value, next_packet_id(packet_id));
    write_register(fd, 0xF0F00614, 0x00000000, next_packet_id(packet_id));
#endif
    fprintf(stdout, "Acquisition stopped\n");
    fflush(stdout);

    //Stop source
    src->exit_flag = 1;
    sleep(1);
    pthread_join(thread_id, NULL);

    //Control Channel Privelege off
    write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x00, next_packet_id(packet_id));

    free(src);
    return fd;
}

