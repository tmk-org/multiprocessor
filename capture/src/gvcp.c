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

void print_packet_full(struct gvcp_packet *packet) {
    struct gvcp_header *header = (struct gvcp_header*)packet; 
    char *bytes = (char*)packet;
    uint32_t i, packet_size = sizeof(struct gvcp_packet) + ntohs(header->size);
    fprintf(stdout, "packet: ");
    fflush(stdout);
    for (i = 0; i < packet_size; i++) {
        fprintf(stdout, "%02hhx ", bytes[i]);
        fflush(stdout);
    }
    fprintf(stdout, "\n");
    fflush(stdout);
}

void print_packet_header(struct gvcp_packet *packet) {
    char *bytes = (char*)packet;
    uint32_t i, packet_size = sizeof(struct gvcp_packet);
    fprintf(stdout, "header: ");
    fflush(stdout);
    for (i = 0; i < packet_size; i++) {
        fprintf(stdout, "%02hhx ", bytes[i]);
        fflush(stdout);
    }
    fprintf(stdout, "\n");
    fflush(stdout);
}

void print_packet_data(struct gvcp_packet *packet) {
    struct gvcp_header *header = (struct gvcp_header*)packet; 
    char *bytes = (char*)packet->data;
    uint32_t i, packet_size = ntohs(header->size);
    fprintf(stdout, "data: ");
    fflush(stdout);
    for (i = 0; i < packet_size; i++) {
        fprintf(stdout, "%02hhx ", bytes[i]);
        fflush(stdout);
    }
    fprintf(stdout, "\n");
    fflush(stdout);
}

struct gvcp_packet *gvcp_discovery_packet(size_t *packet_size) {
    *packet_size = sizeof(struct gvcp_header);
    struct gvcp_packet *packet = (struct gvcp_packet *)malloc(*packet_size); 
    //magic from aravis (see wireshark): protocol GVCP(GigE Vision Communication Protocol) on port 3956
    packet->header.packet_type = GVCP_PACKET_TYPE_CMD;
    packet->header.packet_flags = GVCP_CMD_PACKET_FLAGS_ACK_REQUIRED;
    packet->header.command = htons(GVCP_COMMAND_DISCOVERY_CMD);
    packet->header.size = htons(0x0000);
    packet->header.id = htons(0xffff);
    return packet;
}

int gvcp_discovery_ack(char *vendor, char *model, char *serial, char *user_id, char *mac, struct gvcp_packet *packet) {
    if (!packet) return -1;
    struct gvcp_header *header = &packet->header;
    char *data = packet->data;
    if ((ntohs(header->command) == GVCP_COMMAND_DISCOVERY_ACK) && (ntohs(header->id) == 0xffff)){
        if (vendor) strcpy(vendor, data + GVCP_MANUFACTURER_NAME_OFFSET);
        if (model) strcpy(model, data + GVCP_MODEL_NAME_OFFSET);
        if (serial) strcpy(serial, data + GVCP_SERIAL_NUMBER_OFFSET);
        if (user_id) strcpy(user_id, data + GVCP_USER_DEFINED_NAME_OFFSET);
        if (mac) sprintf(mac, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx", 
                data[GVCP_DEVICE_MAC_ADDRESS_HIGH_OFFSET + 2],
                data[GVCP_DEVICE_MAC_ADDRESS_HIGH_OFFSET + 3],
                data[GVCP_DEVICE_MAC_ADDRESS_HIGH_OFFSET + 4],
                data[GVCP_DEVICE_MAC_ADDRESS_HIGH_OFFSET + 5],
                data[GVCP_DEVICE_MAC_ADDRESS_HIGH_OFFSET + 6],
                data[GVCP_DEVICE_MAC_ADDRESS_HIGH_OFFSET + 7]);
    }
    return 0;
}

struct gvcp_packet *gvcp_read_register_packet(uint32_t reg_addr, uint32_t packet_id, uint32_t *packet_size) {
    *packet_size = sizeof(struct gvcp_header) + sizeof(uint32_t);
    struct gvcp_packet *packet = (struct gvcp_packet *)malloc(*packet_size); 
    //protocol GVCP(GigE Vision Communication Protocol) on port 3956
    packet->header.packet_type = GVCP_PACKET_TYPE_CMD;
    packet->header.packet_flags = GVCP_CMD_PACKET_FLAGS_ACK_REQUIRED;
    packet->header.command = htons(GVCP_COMMAND_READ_REG_CMD);
    packet->header.size = htons(sizeof(uint32_t));
    packet->header.id = htons(packet_id);

    uint32_t n_addr = htonl(reg_addr);
    memcpy(&packet->data, &n_addr, sizeof(uint32_t));

    fprintf(stdout, "[readreg]: command = 0x%04x reg_addr = 0x%04x\n", ntohs(packet->header.command), reg_addr);
    fflush(stdout);
    return packet;
}

struct gvcp_packet *gvcp_write_register_packet(uint32_t reg_addr, uint32_t reg_value, uint32_t packet_id, uint32_t *packet_size) {
    *packet_size = sizeof(struct gvcp_header) + 2 * sizeof(uint32_t);
    struct gvcp_packet *packet = (struct gvcp_packet *)malloc(*packet_size); 
    //protocol GVCP(GigE Vision Communication Protocol) on port 3956
    packet->header.packet_type = GVCP_PACKET_TYPE_CMD;
    packet->header.packet_flags = GVCP_CMD_PACKET_FLAGS_ACK_REQUIRED;
    packet->header.command = htons(GVCP_COMMAND_WRITE_REG_CMD);
    packet->header.size = htons(2 * sizeof(uint32_t));
    packet->header.id = htons(packet_id);

    uint32_t n_addr = htonl(reg_addr);
    memcpy(&packet->data[0], &n_addr, sizeof(uint32_t));
    uint32_t n_value = htonl(reg_value);
    memcpy(&packet->data[sizeof(uint32_t)], &n_value, sizeof(uint32_t));

    fprintf(stdout, "[writereg]: command = 0x%04x reg_addr = 0x%04x value = 0x%04x\n", ntohs(packet->header.command), reg_addr, reg_value);
    fflush(stdout);
    return packet;
}

int gvcp_register_ack(uint32_t *value, struct gvcp_packet *packet) {
    if (!value || !packet) return -1;
    struct gvcp_header *header = &packet->header;
    char *data = (char *)packet->data;
    //print_packet_data(packet);
    if (ntohs(header->packet_type) == GVCP_PACKET_TYPE_ACK){
        *value = ntohl(*(uint32_t*)data);
    }
    fprintf(stdout, "[ack]: command = 0x%04x value = 0x%04x\n", ntohs(header->command), *value);
    fflush(stdout);
    return 0;
}

struct gvcp_packet *listen_packet_ack(int fd) {
    struct sockaddr device_addr;
    socklen_t device_addr_len = sizeof(struct sockaddr);
    fd_set readfd;
    int rc = 0;
    ssize_t bytes = 0;
    struct timeval tv;
    while (fd != -1) {
        FD_ZERO(&readfd);
        FD_SET(fd, &readfd);
        tv.tv_sec = 0;
        tv.tv_usec = 500000;
        rc = select(fd + 1, &readfd, NULL, NULL, &tv);
        if (rc > 0) {
            if (FD_ISSET(fd, &readfd)) {
                uint32_t packet_size = sizeof(struct gvcp_header);
                //for read and write register
                packet_size += sizeof(uint32_t);
                struct gvcp_packet *packet = (struct gvcp_packet*)malloc(packet_size);
                bytes = recvfrom(fd, packet, packet_size, 0, &device_addr, &device_addr_len);
                if (bytes < 0) {
                    fprintf(stdout, "[listen_packet_ack]: socket error\n");
                    fflush(stdout);
                    free(packet);
                    return NULL;
                }
                if (bytes < packet_size) {
                    fprintf(stdout, "[listen_packet_ack]: wrong on inteface\n");
                    fflush(stdout);
                    continue;
                }
                return packet;
            }
        }
        else {
            usleep(100);
            continue;
            //return NULL;
        }
    }
    return NULL;
}

//important: read/write commands are possible only after connect to command socket
int read_register(int fd, uint32_t reg_addr, uint32_t *value, uint32_t packet_id) {
    uint32_t packet_size = 0;
    struct gvcp_packet *packet = gvcp_read_register_packet(reg_addr, packet_id, &packet_size);
    //print_packet_full(packet);
    int rc = write(fd, packet, packet_size);
    free(packet);
    //usleep(500);
    packet = listen_packet_ack(fd);
    gvcp_register_ack(value, packet);
    //print_packet_full(packet);
    free(packet);
    return rc;
}

//important: read/write commands are possible only after connect to command socket
int write_register(int fd, uint32_t reg_addr, uint32_t value, uint32_t packet_id) {
    uint32_t packet_size = 0;
    struct gvcp_packet *packet = gvcp_write_register_packet(reg_addr, value, packet_id, &packet_size);
    //print_packet_full(packet);
    int rc = write(fd, packet, packet_size);
    free(packet);
    //usleep(500);
    packet = listen_packet_ack(fd);
    gvcp_register_ack(&value, packet);
    //print_packet_full(packet);
    free(packet);
    return rc;
}

uint64_t gvcp_get_timestamp_tick_frequency(int fd, uint32_t packet_id) {
    //Read camera parameters
    uint32_t timestamp_tick_frequency_high;
    uint32_t timestamp_tick_frequency_low;

    read_register(fd, GV_TIMESTAMP_TICK_FREQUENCY_HIGH_OFFSET, &timestamp_tick_frequency_high, next_packet_id(packet_id));
    read_register(fd, GV_TIMESTAMP_TICK_FREQUENCY_LOW_OFFSET, &timestamp_tick_frequency_low, next_packet_id(packet_id));
    return ((uint64_t) timestamp_tick_frequency_high << 32) | timestamp_tick_frequency_low;
}

uint32_t gvcp_get_packet_size(int fd, uint32_t packet_id) {
    //Read camera parameters
    uint32_t packet_size;
    read_register(fd, GV_STREAM_CHANNEL_0_PACKET_SIZE_OFFSET, &packet_size, next_packet_id(packet_id));
    return packet_size;
}

uint32_t gvcp_init(int fd, uint32_t packet_id, int data_fd) {
    uint32_t value = -1;
    uint32_t packet_size = 0;

    uint32_t ip, port;
    get_net_parameters_from_socket(data_fd, &ip, &port);

    //Read a number of stream channels register from the camera socket
    read_register(fd, GV_REGISTER_N_STREAM_CHANNELS_OFFSET, &value, packet_id);
    read_register(fd, GV_GVCP_CAPABILITY_OFFSET, &value, next_packet_id(packet_id));

    //Control Channel Privelege on
    write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x02, next_packet_id(packet_id));
    //Write Stream Channel 0 Destination address, port and packet_size
    write_register(fd, GV_STREAM_CHANNEL_0_IP_ADDRESS_OFFSET, ip, next_packet_id(packet_id));
    write_register(fd, GV_STREAM_CHANNEL_0_PORT_OFFSET, port, next_packet_id(packet_id));
    read_register(fd, GV_STREAM_CHANNEL_0_PACKET_SIZE_OFFSET, &packet_size, next_packet_id(packet_id));

    //return packet_size from channel
    return packet_size;
}

#if 0 //fake -- ok
int gvcp_watchdog(int fd, uint32_t packet_id) {
    uint32_t value = -1;
    read_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, &value, next_packet_id(packet_id));
    if (value != 0x02) write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x02, next_packet_id(packet_id)); 
    //read_register(fd, GV_STREAM_CHANNEL_SOURCE_PORT_OFFSET, &value, next_packet_id(packet_id));
    return 0;
}

int gvcp_start(int fd, uint32_t packet_id) {
    uint32_t value = -1;
    write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x02, next_packet_id(packet_id)); 
    write_register(fd, 0x00000124, 0x01, next_packet_id(packet_id)); 
    //read_register(fd, GV_STREAM_CHANNEL_SOURCE_PORT_OFFSET, &value, next_packet_id(packet_id));
    return 0;
}

int gvcp_stop(int fd, uint32_t packet_id) {
    uint32_t value = -1;
    write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x02, next_packet_id(packet_id)); 
    write_register(fd, 0x00000124, 0x00, next_packet_id(packet_id)); 
    return 0;
}
#endif

#if 1 //flir-color  -- ok
int gvcp_watchdog(int fd, uint32_t packet_id) {
    uint32_t value = -1;
    read_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, &value, next_packet_id(packet_id));
    if (value != 0x02) write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x02, next_packet_id(packet_id));
    write_register(fd, 0xF0F04030, 0x80000000, next_packet_id(packet_id)); 
    read_register(fd, GV_STREAM_CHANNEL_SOURCE_PORT_OFFSET, &value, next_packet_id(packet_id));
    return 0;
}

int gvcp_start(int fd, uint32_t packet_id) {
    uint32_t value = -1;
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

    read_register(fd, GV_STREAM_CHANNEL_0_PORT_OFFSET, &value, next_packet_id(packet_id)); //+
    read_register(fd,  GV_STREAM_CHANNEL_SOURCE_PORT_OFFSET, &value, next_packet_id(packet_id));
    write_register(fd, 0xF0F04030, 0x80000000, next_packet_id(packet_id));
    return 0;
}

int gvcp_stop(int fd, uint32_t packet_id) {
    uint32_t value = -1;
    write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x02, next_packet_id(packet_id)); 
    read_register(fd,  0xF0F00614, &value, next_packet_id(packet_id));
    write_register(fd, 0xF0F00614, 0x00000000, next_packet_id(packet_id));
    return 0;
}
#endif

#if 0 //lucid polar -- ok
int gvcp_watchdog(int fd, uint32_t packet_id) {
    uint32_t value = -1;
    read_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, &value, next_packet_id(packet_id));
    if (value != 0x02) write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x02, next_packet_id(packet_id)); 
    //read_register(fd, GV_STREAM_CHANNEL_SOURCE_PORT_OFFSET, &value, next_packet_id(packet_id));
    return 0;
}

int gvcp_start(int fd, uint32_t packet_id) {
    uint32_t value = -1;
    //write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x02, next_packet_id(packet_id)); 
    write_register(fd, 0x10300004, 0x01, next_packet_id(packet_id));
    return 0;
}

int gvcp_stop(int fd, uint32_t packet_id) {
    uint32_t value = -1;
    read_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, &value, next_packet_id(packet_id)); 
    write_register(fd, 0x10300004, 0x00, next_packet_id(packet_id));
    return 0;
}
#endif

#if 0 //flir polar -- doesn't work
int gvcp_watchdog(int fd, uint32_t packet_id) {
    uint32_t value = -1;
    read_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, &value, next_packet_id(packet_id));
    //if (value != 0x02) write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x02, next_packet_id(packet_id)); 
    //read_register(fd, GV_STREAM_CHANNEL_SOURCE_PORT_OFFSET, &value, next_packet_id(packet_id));
    return 0;
}

int gvcp_start(int fd, uint32_t packet_id) {
    uint32_t value = -1;
    write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x02, next_packet_id(packet_id));
    write_register(fd, 0x00081044, 0x00, next_packet_id(packet_id));
    write_register(fd, 0x00081024, 0x00, next_packet_id(packet_id));
    write_register(fd, 0x00081084, 0xc8, next_packet_id(packet_id));
    write_register(fd, 0x00081064, 0xc8, next_packet_id(packet_id));
/*
    read_register(fd, 0x000c110c, &value, next_packet_id(packet_id));
    read_register(fd, 0x000c1148, &value, next_packet_id(packet_id));
    read_register(fd, 0x000c1108, &value, next_packet_id(packet_id));
    read_register(fd, 0x000c8c40, &value, next_packet_id(packet_id));
    read_register(fd, 0x000c8c20, &value, next_packet_id(packet_id));
    read_register(fd, 0x000c8c00, &value, next_packet_id(packet_id));
    write_register(fd, 0x000c8424, 0x00, next_packet_id(packet_id));
    write_register(fd, 0xc1124, 0x01, next_packet_id(packet_id));
    read_register(fd, 0x000c1148, &value, next_packet_id(packet_id));
    write_register(fd, 0x000c1104, 0xbebc20, next_packet_id(packet_id)); //wtf
    read_register(fd, 0x20002008, &value, next_packet_id(packet_id));
*/

    write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x02, next_packet_id(packet_id));
    write_register(fd, GV_STREAM_CHANNEL_0_PORT_OFFSET, port, next_packet_id(packet_id));
    read_register(fd,  GV_STREAM_CHANNEL_SOURCE_PORT_OFFSET, &value, next_packet_id(packet_id));
    read_register(fd, GV_STREAM_CHANNEL_0_PACKET_SIZE_OFFSET, &src->packet_size, next_packet_id(packet_id));

     write_register(fd, 0x00004050, 0, next_packet_id(packet_id));
     write_register(fd, 0x00004054, 0, next_packet_id(packet_id));

    write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x02, next_packet_id(packet_id));
        write_register(fd, 0xc00c8, 0x00, next_packet_id(packet_id));
        write_register(fd, 0xc1104, 0x01, next_packet_id(packet_id));
        write_register(fd, 0xc0004, 0x01, next_packet_id(packet_id));
    return 0;
}

int gvcp_stop(int fd, uint32_t packet_id) {
    uint32_t value = -1;
    write_register(fd, GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET, 0x02, next_packet_id(packet_id)); 
    write_register(fd, 0xc1124, 0x00, next_packet_id(packet_id)); 
    return 0;
}
#endif

//TODO: gvcp_genicam_xml()

//TODO: gvcp_apply_command_sequence()




