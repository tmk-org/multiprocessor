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
//usleep
#include <unistd.h>

#include "capture/list.h"

#include "capture/gvcp.h"
#include "capture/gvsp.h"
#include "capture/device.h"
#include "capture/camera.h"

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
    
    uint32_t n_packets;
    uint32_t error_packets_received;

    uint32_t last_valid_packet;
    struct packet_data *packet_data;
    
    uint64_t first_packet_time_us;
    uint64_t last_packt_time_us;

    //???
    int n_packets_resent;
    int resend_ratio_reached;
};

void* gvsp_thread(void *arg) {
    int extended_ids = 0;
    uint32_t packet_id;
    uint64_t frame_id;
    uint64_t last_frame_id = 0;
    uint32_t i; //counter for packets
    struct gige_camera *cam = (struct gige_camera *)arg;
    int fd = cam->gvsp_fd;
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

    uint32_t packet_size = cam->packet_size - IP_HEADER_SIZE - UDP_HEADER_SIZE;
    struct gvsp_packet *packet = (struct gvsp_packet*)malloc(packet_size);

    while (cam->state == STREAM_LIVE && cam->gvsp_fd != -1) {
        //fprintf(stdout, "[gvsp_thread]: [1] Hello from source_data_thread_udp\n");
        //fflush(stdout);
        FD_ZERO(&readfd);
        FD_SET(fd, &readfd);
        tv.tv_sec = 0;
        tv.tv_usec = 500000;
        rc = select(fd + 1, &readfd, NULL, NULL, &tv);
        if (rc > 0) {
            if (FD_ISSET(fd, &readfd)) {
                //fprintf(stdout, "[gvsp_thread]: [2] Hello from source_data_thread_udp before read\n");
                //fflush(stdout);
                bytes = recvfrom(fd, packet, packet_size, 0, &device_addr, &device_addr_len);
                //fprintf(stdout, "[gvsp_thread]: bytes = %ld, packet_size = %d\n", bytes, packet_size);
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
                //fprintf(stdout, "[gvsp_threadp]: total = %d ", n_received_packets);
                //fprintf(stdout, "packet_id = 0x%hx(%d), frame_id = 0x%lx(%lu)\n", packet_id, packet_id, frame_id, frame_id);
                //fflush(stdout);

                //find frame data
                //now is primitive:  check that frame_id > last_frame_id
                if (frame_id > last_frame_id) {
                    if (frame->is_started) {
                        if (cam->new_frame_action) cam->new_frame_action(frame->buffer, frame->real_size, (void*)&last_frame_id);
                    }
                    fprintf(stdout, "[gvsp_thread]: new frame %lu found\n", frame_id);
                    fflush(stdout);
                    frame->real_size = 0;
                    frame->frame_id = frame_id;
                    frame->last_valid_packet = -1;
                    frame->has_extended_ids = extended_ids;
                    frame->is_started = 1;
                    frame->n_packets = 0;
                    frame->error_packets_received = 0;
                    last_frame_id = frame->frame_id;
                }

                if (frame->is_started) {
                    enum gvsp_packet_type packet_type = gvsp_packet_get_packet_type(packet);
                    if (gvsp_packet_type_is_error(packet_type)) {
                        //TODO:
                        fprintf(stdout, "[gvsp_thread]: error packet recieved\n");
                        fflush(stdout);
                        n_error_packets++;
                        frame->error_packets_received++;
                    }
                    //TODO: think about necessary of this case
                    else if (packet_id < frame->n_packets && frame->packet_data[packet_id].received) {
                        fprintf(stdout, "[gvsp_thread]: very strange -- duplicate packet recieved\n");
                        fflush(stdout);
                    }
                    else {
                        //fprintf(stdout, "[gvsp_thread]: normal packet recieved\n");
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
                                    fprintf(stdout, "[gvsp_thread]: image data (timestamp = %lu): width = %u, height = %u, offset_x = %u, offset_y = %u, pixel_format = 0x%x\n", gvsp_packet_get_timestamp(packet, cam->timestamp_tick_frequency), 
                                            gvsp_packet_get_width(packet),
                                            gvsp_packet_get_height(packet),
                                            gvsp_packet_get_x_offset(packet),
                                            gvsp_packet_get_y_offset(packet),
                                            gvsp_packet_get_pixel_format(packet));
                                    fflush(stdout);
                                }
                                else {
                                    fprintf(stdout, "[gvsp_thread]: strange payload type\n");
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
                                //fprintf(stdout, "[gvsp_thread]: frame->real_size = %zd\n", frame->real_size);
                                //fflush(stdout);
                                break;
                            }
                            case GVSP_CONTENT_TYPE_DATA_TRAILER:
#if 0
                                if (frame->error_packets_received > 0) {
                                    fprintf(stdout, "[gvsp_thread]: %d error packets recived\n", frame->error_packets_received);
                                    fflush(stdout);
                                }
                                else {
                                    fprintf(stdout, "[gvsp_thread]: no error packets recieved\n");
                                    fflush(stdout);
                                }
#endif
                                fprintf(stdout, "[gvsp_thread]: frame->real_size = %zd\n", frame->real_size);
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
            fprintf(stdout, "[gvsp_thread]: no data on inteface\n");
            fflush(stdout);
#endif
            usleep(100);
            continue;
        }
    }
    cam->last_frame_id = frame_id;
    cam->n_received_packets = n_received_packets;
    cam->n_ignored_packets = n_ignored_packets;
    cam->n_error_packets = n_error_packets;
    free(packet);
    free(frame);
    fprintf(stdout, "[gvsp_thread]: stop\n");
    fflush(stdout);
    return NULL;
}

//ATTENTION: 'dev' should be not null before init
struct gige_camera *gige_camera_init(struct device *dev, void (*new_frame_action)(void *, unsigned int, void *)) {

    struct gige_camera *cam = (struct gige_camera *)malloc(sizeof(struct gige_camera));

    if (!dev || !cam) {
        fprintf(stderr, "[gige_camera_init]: cam or link to device is NULL\n");
        fflush(stderr);
        return NULL;
    }

    cam->dev = dev;
    cam->new_frame_action = new_frame_action;
    cam->gvcp_fd = -1;
    cam->gvsp_fd = -1;
    //0xf000 is magic (it should be uint16_t, not 0x0000 and numbers should be increasing)
    cam->last_gvcp_packet_id = 0xf000;

    int fd = -1, rc = 0, opt = 1;
    uint32_t value = -1;

    struct sockaddr_in device_addr;

    uint32_t packet_id = 0xf000;
    uint32_t packet_size = GVSP_PACKET_SIZE_DEFAULT;

    //open command socket (GVCP)
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(fd < 0) {
        fprintf(stdout, "command socket error\n");
        fflush(stdout);
        return NULL;
    }

    rc = bind(fd, dev->iface_addr, sizeof(struct sockaddr_in));
    if (rc < 0) {
        fprintf(stdout, "Couldn't bind the command socket\n");
        fflush(stdout);
        return NULL;
    }

    opt = DEVICE_BUFFER_SIZE;
    setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &opt, sizeof(opt));

    //connect is neccessary for directly write packets to the command socket
    device_addr.sin_family = AF_INET;
    device_addr.sin_addr.s_addr = inet_addr(dev->ip);;
    device_addr.sin_port = htons(atoi(dev->port));
    rc = connect(fd, (struct sockaddr*)&device_addr, sizeof(struct sockaddr_in));
    if (rc) {
        fprintf(stdout, "Couldn't connect to the command socket\n");
        fflush(stdout);
        return NULL;
    }

    cam->gvcp_fd = fd;

    //open data socket (GVSP)
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(fd < 0) {
        fprintf(stdout, "data socket error\n");
        fflush(stdout);
        close(cam->gvcp_fd);
        return NULL;
    }
    rc = bind(fd, dev->iface_addr, sizeof(struct sockaddr_in));
    if (rc < 0) {
        fprintf(stdout, "bind command socket error\n");
        fflush(stdout);
        close(cam->gvcp_fd);
        close(fd);
        return NULL;
    }
    opt = GV_STREAM_INCOMING_BUFFER_SIZE;
    setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &opt, sizeof(opt));

    //gvcp_init returns packet_size from camera
    if ((cam->packet_size = gvcp_init(cam->gvcp_fd, next_packet_id(cam->last_gvcp_packet_id), fd)) > 0) { //think about test camera with acquisition
        cam->gvsp_fd = fd;
#if 0 //wtf, why it's doesn't work when start in this place
        pthread_create(&(cam->data_thread_id), NULL, gvsp_thread, cam);
        fprintf(stdout, "Data thread created on fd = %d\n", cam->gvsp_fd);
        fflush(stdout);
#endif
        return cam;
    }

    return NULL;
}

void gige_camera_destroy(struct gige_camera *cam) {
#if 0
    cam->state = STREAM_STOP;
    sleep(1);
    cam->new_frame_action = NULL;
    pthread_join(cam->data_thread_id, NULL);
#endif
    close(cam->gvsp_fd);
    close(cam->gvcp_fd);
    if (cam->dev) free(dev);
    //free(camera);  -- this action should be called from place where were malloc()
}

int gige_camera_test(struct gige_camera *cam) {
    int cnt = 0;
    gvcp_start(cam->gvcp_fd, next_packet_id(cam->last_gvcp_packet_id));
        cam->state = STREAM_LIVE;
#if 1
        pthread_create(&(cam->data_thread_id), NULL, gvsp_thread, cam);
        fprintf(stdout, "Data thread created on fd = %d\n", cam->gvsp_fd);
        fflush(stdout);
#endif
    while(cnt < 10) {
        gvcp_watchdog(cam->gvcp_fd, next_packet_id(cam->last_gvcp_packet_id));
        sleep(1);
        cnt++;
    }
    gvcp_stop(cam->gvcp_fd, next_packet_id(cam->last_gvcp_packet_id));
    cam->state = STREAM_STOP;
    close(cam->gvcp_fd);
    cam->gvcp_fd = -1;
#if 1
    sleep(1);
    cam->new_frame_action = NULL;
    pthread_join(cam->data_thread_id, NULL);
    close(cam->gvsp_fd);
    cam->gvsp_fd = -1;
#endif
    return 0;
}


