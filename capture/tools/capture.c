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

#include "capture/list.h"

#include <pthread.h>

//https://rsdn.org/article/unix/sockets.xml

/********************* *******************************/
/* sudo /sbin/ifconfig enp1s0:3 169.254.3.100/24 up  */
/* sudo /sbin/ifconfig enp1s0:2 169.254.2.100/24 up  */
/* bin/arvsource enp1s0:2 test:2  --- fake-1         */
/* bin/arvclient 169.254.2.100                       */
/********************* *******************************/

//_discover
//gvcp_gv_discover_socket_list_new
//gvcp_enumerate_network_interfaces

//magic const from aravis
#define SOCKET_DISCOVER_BUFFER_SIZE 256*1024
#define DEVICE_BUFFER_SIZE          512

#define GV_STREAM_INCOMING_BUFFER_SIZE  65536

#define GVCP_DEVICE_PORT                 3956 //GVCP
#define GVCP_DISCOVERY_DATA_SIZE        0xf8

#define GVCP_MANUFACTURER_NAME_OFFSET   0x00000048
#define GVCP_MANUFACTURER_NAME_SIZE     32

#define GVCP_MODEL_NAME_OFFSET          0x00000068
#define GVCP_MODEL_NAME_SIZE            32

#define GVCP_DEVICE_VERSION_OFFSET      0x00000088
#define GVCP_DEVICE_VERSION_SIZE        32

#define GVCP_MANUFACTURER_INFORMATIONS_OFFSET   0x000000a8
#define GVCP_MANUFACTURER_INFORMATIONS_SIZE     48

#define GVCP_SERIAL_NUMBER_OFFSET       0x000000d8
#define GVCP_SERIAL_NUMBER_SIZE	        16

#define GVCP_USER_DEFINED_NAME_OFFSET   0x000000e8
#define GVCP_USER_DEFINED_NAME_SIZE     16

#define GVCP_DEVICE_MAC_ADDRESS_HIGH_OFFSET     0x00000008

//stream parameters
#define GVSP_PACKET_SIZE_DEFAULT            1500
#define GVSP_PACKET_EXTENDED_ID_MODE_MASK   0x80
#define GVSP_PACKET_ID_MASK                 0x00ffffff
#define GVSP_PACKET_INFO_CONTENT_TYPE_MASK  0x7f000000
#define GVSP_PACKET_INFO_CONTENT_TYPE_POS   24

//register addresses
#define GV_GVCP_CAPABILITY_OFFSET               0x00000934


#define GV_REGISTER_N_STREAM_CHANNELS_OFFSET    0x00000904

#define GV_HEARTBEAT_TIMEOUT_OFFSET             0x00000938
#define GV_TIMESTAMP_TICK_FREQUENCY_HIGH_OFFSET   0x0000093c
#define GV_TIMESTAMP_TICK_FREQUENCY_LOW_OFFSET    0x00000940
#define GV_TIMESTAMP_CONTROL_OFFSET             0x00000944

#define GV_STREAM_CHANNEL_0_IP_ADDRESS_OFFSET   0x00000d18
#define GV_STREAM_CHANNEL_0_PORT_OFFSET         0x00000d00
#define GV_STREAM_CHANNEL_0_PACKET_SIZE_OFFSET  0x00000d04
#define GV_STREAM_CHANNEL_SOURCE_PORT_OFFSET    0x00000d1c

#define GV_CONTROL_CHANNEL_PRIVILEGE_OFFSET     0x00000a00

#define GV_XML_URL_0_OFFSET                     0x00000200
#define GV_XML_URL_1_OFFSET                     0x00000400
#define GV_XML_URL_SIZE                         512
//#define ARV_GVBS_STREAM_CHANNEL_0_PACKET_SIZE_MASK		0x0000ffff
//#define ARV_GVBS_STREAM_CHANNEL_0_PACKET_SIZE_POS		0
//#define ARV_GVBS_STREAM_CHANNEL_0_PACKET_BIG_ENDIAN		1 << 29
//#define ARV_GVBS_STREAM_CHANNEL_0_PACKET_DO_NOT_FRAGMENT	1 << 30
//#define ARV_GVBS_STREAM_CHANNEL_0_PACKET_SIZE_FIRE_TEST		1 << 31

//#define ARV_GVBS_STREAM_CHANNEL_0_PACKET_DELAY_OFFSET		0x00000d08

//#define ARV_GVBS_STREAM_CHANNEL_0_IP_ADDRESS_OFFSET		0x00000d18

//#define ARV_GVBS_STREAM_CHANNEL_0_PACKET_SIZE_MIN_OFFSET		0x0000c000
//#define ARV_GVBS_STREAM_CHANNEL_0_PACKET_SIZE_MAX_OFFSET		0x0000c004

//#define ARV_GVCP_DATA_SIZE_MAX				512

#define IP_HEADER_SIZE      20
#define UDP_HEADER_SIZE     8

#define MAX_FRAME_SIZE (2048 * 2448 * 3 * 2)

//arvgvcpprivate.h / arvgvcp.c
enum gvcp_command {
    GVCP_COMMAND_DISCOVERY_CMD = 0x0002,
    GVCP_COMMAND_DISCOVERY_ACK = 0x0003,
    GVCP_COMMAND_PACKET_RESEND_CMD = 0x0040,
    GVCP_COMMAND_PACKET_RESEND_ACK = 0x0041,
    GVCP_COMMAND_READ_REG_CMD = 0x0080,
    GVCP_COMMAND_READ_REG_ACK = 0x0081,
    GVCP_COMMAND_WRITE_REG_CMD = 0x0082,
    GVCP_COMMAND_WRITE_REG_ACK = 0x0083,
    GVCP_COMMAND_READ_MEM_CMD = 0x0084,
    GVCP_COMMAND_READ_MEM_ACK = 0x0085,
    GVCP_COMMAND_WRITE_MEM_CMD = 0x0086,
    GVCP_COMMAND_WRITE_MEM_ACK = 0x0087
};

enum gvcp_packet_type {
    GVCP_PACKET_TYPE_ACK = 0x00,
    GVCP_PACKET_TYPE_CMD = 0x42,
    GVCP_PACKET_TYPE_ERROR = 0x80,
    GVCP_PACKET_TYPE_UNKNOWN_ERROR = 0x8f
};

enum gvcp_cmd_packet_flags {
    GVCP_CMD_PACKET_FLAGS_NONE = 0x00,
    GVCP_CMD_PACKET_FLAGS_ACK_REQUIRED = 0x01,
    GVCP_CMD_PACKET_FLAGS_EXTENDED_IDS = 0x10
};

struct gvcp_header{
    uint8_t packet_type;
    uint8_t packet_flags;
    uint16_t command;
    uint16_t size;
    uint16_t id;
};

struct gvcp_packet {
    struct gvcp_header header;
    char data[];
};

enum gvsp_content_type {
    GVSP_CONTENT_TYPE_DATA_LEADER = 0x01,
    GVSP_CONTENT_TYPE_DATA_TRAILER = 0x02,
    GVSP_CONTENT_TYPE_DATA_BLOCK = 0x03,
    GVSP_CONTENT_TYPE_ALL_IN = 0x04
};

enum gvsp_payload_type {
    GVSP_PAYLOAD_TYPE_UNKNOWN = -1,
    GVSP_PAYLOAD_TYPE_IMAGE = 0x0001,
    GVSP_PAYLOAD_TYPE_RAWDATA = 0x0002,
    GVSP_PAYLOAD_TYPE_FILE = 0x0003,
    GVSP_PAYLOAD_TYPE_CHUNK_DATA = 0x0004,
    GVSP_PAYLOAD_TYPE_EXTENDED_CHUNK_DATA = 0x0005, //Deprecated by AIA GigE Vision Specification v2.0
    GVSP_PAYLOAD_TYPE_JPEG = 0x0006,
    GVSP_PAYLOAD_TYPE_JPEG2000 = 0x0007,
    GVSP_PAYLOAD_TYPE_H264 = 0x0008,
    GVSP_PAYLOAD_TYPE_MULTIZONE_IMAGE = 0x0009,
    GVSP_PAYLOAD_TYPE_IMAGE_EXTENDED_CHUNK = 0x4001
};

enum gvsp_packet_type {
    GVSP_PACKET_TYPE_OK = 0x0000,
    GVSP_PACKET_TYPE_RESEND = 0x0100,
    GVSP_PACKET_TYPE_PACKET_UNAVAILABLE = 0x800c
};

struct gvsp_data_leader {
    uint16_t flags;
    uint16_t payload_type;
    uint32_t timestamp_high;
    uint32_t timestamp_low;
    uint32_t pixel_format;
    uint32_t width;
    uint32_t height;
    uint32_t x_offset;
    uint32_t y_offset;
};

//__attribute__((packed)) used for alignement as is (any way wrong offset of packet_id)

struct gvsp_header {
    uint16_t frame_id;      //= block_id
    uint32_t packet_info;   //= payload_type (1 byte) + packet_id (3 bytes)
    uint8_t data[];
} __attribute__((packed));

struct gvsp_extended_header {
    uint16_t flags;
    uint32_t packet_info;
    uint64_t frame_id;
    uint32_t packet_id;
    uint8_t data[];
} __attribute__((packed));

struct gvsp_packet {
    uint16_t packet_type;   //= packet_status
    uint8_t header[];
} __attribute__((packed));

void dump_data(const char *prefix, char *bytes, size_t size) {
    uint32_t i;
    fprintf(stdout, "%s: ", prefix);
    fflush(stdout);
    if (!bytes) {
        fprintf(stdout, "<null>");
        fflush(stdout);
    }
    for (i = 0; i < size; i++) {
        fprintf(stdout, "%02hhx ", bytes[i]);
        fflush(stdout);
    }
    fprintf(stdout, "\n");
    fflush(stdout);
}

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

int gvsp_packet_type_is_error(const enum gvcp_packet_type packet_type) {
    return (packet_type == GVCP_PACKET_TYPE_ERROR);
}

enum gvsp_packet_type gvsp_packet_get_packet_type(const struct gvsp_packet *packet) {
    return (const enum gvsp_packet_type)ntohs(packet->packet_type);
}

int gvsp_packet_has_extended_ids(const struct gvsp_packet *packet) {
    return ((packet->header[2] & GVSP_PACKET_EXTENDED_ID_MODE_MASK) != 0);
}

enum gvsp_content_type gvsp_packet_get_content_type(const struct gvsp_packet *packet) {
    if (gvsp_packet_has_extended_ids(packet)) {
        struct gvsp_extended_header *header = (struct gvsp_extended_header *)&packet->header;
        return (enum gvsp_content_type)((ntohl(header->packet_info) & GVSP_PACKET_INFO_CONTENT_TYPE_MASK) >> GVSP_PACKET_INFO_CONTENT_TYPE_POS);
    }
    else {
        struct gvsp_header *header = (struct gvsp_header *)&packet->header;
        return (enum gvsp_content_type)((ntohl(header->packet_info) & GVSP_PACKET_INFO_CONTENT_TYPE_MASK) >> GVSP_PACKET_INFO_CONTENT_TYPE_POS);
    }
}

uint32_t gvsp_packet_get_packet_id(const struct gvsp_packet *packet) {
    //dump_data("packet", packet, 40);
    if (gvsp_packet_has_extended_ids(packet)) {
        struct gvsp_extended_header *header = (struct gvsp_extended_header *)&packet->header;
        return ntohl(header->packet_id);
    }
    else {
        struct gvsp_header *header = (struct gvsp_header *)&packet->header;
        return (ntohl(header->packet_info) & GVSP_PACKET_ID_MASK);
    }
}

uint64_t gvsp_packet_get_frame_id(const struct gvsp_packet *packet) {
    if (gvsp_packet_has_extended_ids(packet)) {
        struct gvsp_extended_header *header = (struct gvsp_extended_header *)&packet->header;
        //return GUINT64_FROM_BE(header->frame_id); -- check this place: convert from big endian
        return header->frame_id;
    }
    else {
        struct gvsp_header *header = (struct gvsp_header *)&packet->header;
        return ntohs(header->frame_id);
    }
}

char *gvsp_packet_get_data(const struct gvsp_packet *packet) {
    if (gvsp_packet_has_extended_ids(packet)) {
        struct gvsp_extended_header *header = (struct gvsp_extended_header *)&packet->header;
        return (char*)&(header->data);
    }
    else {
        struct gvsp_header *header = (struct gvsp_header *)&packet->header;
        return (char*)&(header->data);
    }
}


size_t gvsp_packet_get_data_size(const struct gvsp_packet *packet, size_t packet_size) {
    if (gvsp_packet_has_extended_ids(packet)) {
        return packet_size - sizeof(struct gvsp_packet) - sizeof(struct gvsp_extended_header);
    }
    else {
        return packet_size - sizeof(struct gvsp_packet) - sizeof(struct gvsp_header);
    }
}

enum gvsp_payload_type gvsp_packet_get_payload_type(const struct gvsp_packet *packet) {
    struct gvsp_data_leader *leader = (struct gvsp_data_leader *)gvsp_packet_get_data(packet);
    enum gvsp_payload_type  payload_type = ntohs(leader->payload_type);
    switch (payload_type) {
        case GVSP_PAYLOAD_TYPE_IMAGE:
        case GVSP_PAYLOAD_TYPE_FILE:
            return GVSP_PAYLOAD_TYPE_IMAGE;
        default:
            return GVSP_PAYLOAD_TYPE_UNKNOWN;
    }
    return GVSP_PAYLOAD_TYPE_UNKNOWN;
}

static inline uint32_t gvsp_packet_get_x_offset(const struct gvsp_packet *packet) {
    struct gvsp_data_leader *leader = (struct gvsp_data_leader *)gvsp_packet_get_data(packet);
    return ntohl(leader->x_offset);
}

static inline uint32_t gvsp_packet_get_y_offset(const struct gvsp_packet *packet) {
    struct gvsp_data_leader *leader = (struct gvsp_data_leader *)gvsp_packet_get_data(packet);
    return ntohl(leader->y_offset);
}

static inline uint32_t gvsp_packet_get_width(const struct gvsp_packet *packet) {
    struct gvsp_data_leader *leader = (struct gvsp_data_leader *)gvsp_packet_get_data(packet);
    return ntohl(leader->width);
}

static inline uint32_t gvsp_packet_get_height(const struct gvsp_packet *packet) {
    struct gvsp_data_leader *leader = (struct gvsp_data_leader *)gvsp_packet_get_data(packet);
    return ntohl(leader->height);
}

static inline uint32_t gvsp_packet_get_pixel_format(const struct gvsp_packet *packet) {
    struct gvsp_data_leader *leader = (struct gvsp_data_leader *)gvsp_packet_get_data(packet);
    return ntohl(leader->pixel_format);
}

static inline uint64_t gvsp_packet_get_timestamp(const struct gvsp_packet *packet, uint64_t timestamp_tick_frequency) {
    struct gvsp_data_leader *leader = (struct gvsp_data_leader *)gvsp_packet_get_data(packet);
    uint64_t timestamp_s;
    uint64_t timestamp_ns;
    uint64_t timestamp;

    if (timestamp_tick_frequency < 1) {
        return 0;
    }

    timestamp = ((uint64_t)ntohl(leader->timestamp_high) << 32) | ntohl(leader->timestamp_low);
    timestamp_s = timestamp / timestamp_tick_frequency;
    timestamp_ns = ((timestamp % timestamp_tick_frequency) * 1000000000) / timestamp_tick_frequency;

    timestamp_ns += timestamp_s * 1000000000;

    return timestamp_ns;
}

struct interface {
    int fd;
    char name[80];
    struct sockaddr *addr;
    list_t entry;
};

struct device {
    int fd; //?interface

    int cmd_fd;  //command socket (:3956)
    uint16_t packet_id; //incrementation packet_id for correct sending to the command socket

    int data_fd; //data socket (from connect)

    char ip[16];
    char port[8];
    struct sockaddr_in device_addr;

    char vendor[GVCP_MANUFACTURER_NAME_SIZE + 1];
    char model[GVCP_MODEL_NAME_SIZE + 1];
    char serial[GVCP_SERIAL_NUMBER_SIZE + 1];
    char user_id[GVCP_USER_DEFINED_NAME_SIZE + 1];
    char mac[18];

    char iface_name[80];
    struct sockaddr *iface_addr;
    list_t entry;
};

struct devices {
    int device_cnt;
    list_t device_list;
};

void *get_in_addr(struct sockaddr *saddr) {
    if (saddr->sa_family == AF_INET)
        return &(((struct sockaddr_in*)saddr)->sin_addr);
    return &(((struct sockaddr_in6*)saddr)->sin6_addr);
}

void interface_print(struct interface *iface) {
    char buf[INET6_ADDRSTRLEN];
    if (inet_ntop(iface->addr->sa_family, get_in_addr(iface->addr), buf, sizeof(buf)) != NULL) {
        fprintf(stdout, "Found inteface with name %s ip %s fd = %d\n", iface->name, buf, iface->fd);
    }
    else {
        fprintf(stdout, "inet_ntop error\n");
    }
    fflush(stdout);
}

struct interfaces {
    int interface_cnt;
    list_t interface_list;
};

struct source_data {
    int fd;
    int exit_flag;
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

struct gvcp_packet *prepare_discovery_packet(size_t *packet_size) {
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

int connect_to_interface_for_discovery(struct interface *iface) {
    int fd, rc = 0, opt = 1;
    struct sockaddr_in device_addr;
    //struct udp_header header;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(fd < 0) {
        fprintf(stdout, "socket error\n");
        fflush(stdout);
        return -1;
    }

    device_addr.sin_family = AF_INET;
    device_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    device_addr.sin_port = htons(GVCP_DEVICE_PORT);

    rc = bind(fd, iface->addr, sizeof(struct sockaddr_in));
    if (rc < 0) {
        fprintf(stdout, "bind error\n");
        fflush(stdout);
        return -1;
    }

    opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));
    opt = DEVICE_BUFFER_SIZE;
    setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &opt, sizeof(opt));

    //header.dest_port = htons(0);
    //header.length = htons(sizeof(header)+sizeof(packet.));
    //header.checksum = 0;

    size_t packet_size;
    struct gvcp_packet *packet = prepare_discovery_packet(&packet_size);
    rc = sendto(fd, packet, sizeof(struct gvcp_packet), 0, (struct sockaddr*)&device_addr, sizeof(struct sockaddr_in));
    free(packet);

    return fd;
}

struct gvcp_packet *prepare_gvcp_read_register_packet(uint32_t reg_addr, uint32_t packet_id, uint32_t *packet_size) {
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

struct gvcp_packet *prepare_gvcp_write_register_packet(uint32_t reg_addr, uint32_t reg_value, uint32_t packet_id, uint32_t *packet_size) {
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

int parse_gvcp_register_ack(uint32_t *value, struct gvcp_packet *packet) {
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
    while (1) {
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
                    fprintf(stdout, "[listen_discovery_answer]: no data on inteface\n");
                    fflush(stdout);
                    free(packet);
                    return NULL;
                }
                if (bytes < packet_size) {
                    fprintf(stdout, "[listen_discovery_answer]: wrong on inteface\n");
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

uint16_t next_packet_id(uint16_t packet_id) {
    return 0xf000 | ((++packet_id) & 0xfff);
}

//important: read/write commands are possible only after connect to command socket
int read_register(int fd, uint32_t reg_addr, uint32_t *value, uint32_t packet_id) {
    uint32_t packet_size = 0;
    struct gvcp_packet *packet = prepare_gvcp_read_register_packet(reg_addr, packet_id, &packet_size);
    //print_packet_full(packet);
    int rc = write(fd, packet, packet_size);
    free(packet);
    //usleep(500);
    packet = listen_packet_ack(fd);
    parse_gvcp_register_ack(value, packet);
    //print_packet_full(packet);
    free(packet);
    return rc;
}

//important: read/write commands are possible only after connect to command socket
int write_register(int fd, uint32_t reg_addr, uint32_t value, uint32_t packet_id) {
    uint32_t packet_size = 0;
    struct gvcp_packet *packet = prepare_gvcp_write_register_packet(reg_addr, value, packet_id, &packet_size);
    //print_packet_full(packet);
    int rc = write(fd, packet, packet_size);
    free(packet);
    //usleep(500);
    packet = listen_packet_ack(fd);
    parse_gvcp_register_ack(&value, packet);
    //print_packet_full(packet);
    free(packet);
    return rc;
}

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
        //fprintf(stdout, "[1] Hello from source_data_thread_udp\n");
        //fflush(stdout);
        FD_ZERO(&readfd);
        FD_SET(fd, &readfd);
        tv.tv_sec = 0;
        tv.tv_usec = 500000;
        rc = select(fd + 1, &readfd, NULL, NULL, &tv);
        if (rc > 0) {
            if (FD_ISSET(fd, &readfd)) {
                //fprintf(stdout, "[2] Hello from source_data_thread_udp before read\n");
                //fflush(stdout);
                bytes = recvfrom(fd, packet, packet_size, 0, &device_addr, &device_addr_len);
                //fprintf(stdout, "bytes = %ld, packet_size = %d\n", bytes, packet_size);
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
                //fprintf(stdout, "total = %d ", n_received_packets);
                //fprintf(stdout, "packet_id = 0x%hx(%d), frame_id = 0x%lx(%lu)\n", packet_id, packet_id, frame_id, frame_id);
                //fflush(stdout);

                //find frame data
                //now is primitive:  check that frame_id > last_frame_id
                if (frame_id > last_frame_id) {
                    if (frame->is_started) {
                        char fname[50];
                        sprintf(fname, "data/out_%ld.raw", last_frame_id);
                        fprintf(stdout, "Out frame to %s as binary\n", fname);
                        int out_fd;
                        out_fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC, 0664);
                        if (out_fd > 0) {
                            write(out_fd, frame->buffer, frame->real_size);
                            close(out_fd);
                        }
                    }
                    fprintf(stdout, "New frame %lu found\n", frame_id);
                    fflush(stdout);
                    frame->real_size = 0;
                    frame->frame_id = frame_id;
                    frame->last_valid_packet = -1;
                    frame->has_extended_ids = extended_ids;
                    frame->is_started = 1;
                    frame->n_packets = 0;
                    last_frame_id = frame->frame_id;
                }

                if (frame->is_started) {
                    enum gvsp_packet_type packet_type = gvsp_packet_get_packet_type(packet);
                    if (gvsp_packet_type_is_error(packet_type)) {
                        //TODO:
                        fprintf(stdout, "error packet recieved\n");
                        fflush(stdout);
                        n_error_packets++;
                        frame->error_packet_recieved = 1;
                    }
                    //TODO: think about necessary of this case
                    else if (packet_id < frame->n_packets && frame->packet_data[packet_id].received) {
                        fprintf(stdout, "very strange: duplicate packet recieved\n");
                        fflush(stdout);
                    }
                    else {
                        //fprintf(stdout, "normal packet recieved\n");
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
                                    fprintf(stdout, "Image data (timestamp = %lu): width = %u, height = %u, offset_x = %u, offset_y = %u, pixel_format = 0x%x\n", gvsp_packet_get_timestamp(packet, src->timestamp_tick_frequency), 
                                            gvsp_packet_get_width(packet),
                                            gvsp_packet_get_height(packet),
                                            gvsp_packet_get_x_offset(packet),
                                            gvsp_packet_get_y_offset(packet),
                                            gvsp_packet_get_pixel_format(packet));
                                    fflush(stdout);
                                }
                                else {
                                    fprintf(stdout, "Strange payload type\n");
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
                                if (frame->error_packet_recieved == 1) {
                                    fprintf(stdout, "1 ");
                                }
                                else {
                                    fprintf(stdout, "0 ");
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

int connect_to_device(struct device *dev) {
    pthread_t thread_id;
    int fd = -1, data_fd = -1, rc = 0, opt = 1;
    uint32_t value = -1;
    struct sockaddr_in device_addr;
    uint32_t packet_id = 0xf000;
    uint32_t packet_size = 0x578;

    //open command socket (GVCP)
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(fd < 0) {
        fprintf(stdout, "command socket error\n");
        fflush(stdout);
        return -1;
    }

    rc = bind(fd, dev->iface_addr, sizeof(struct sockaddr_in));
    if (rc < 0) {
        fprintf(stdout, "Couldn't bind the command socket\n");
        fflush(stdout);
        return -1;
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
        return -1;
    }

    //open data socket (GVSP)
    data_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(data_fd < 0) {
        fprintf(stdout, "data socket error\n");
        fflush(stdout);
        return -1;
    }
    rc = bind(data_fd, dev->iface_addr, sizeof(struct sockaddr_in));
    if (rc < 0) {
        fprintf(stdout, "bind command socket error\n");
        fflush(stdout);
        return -1;
    }
    opt = GV_STREAM_INCOMING_BUFFER_SIZE;
    setsockopt(data_fd, SOL_SOCKET, SO_RCVBUF, &opt, sizeof(opt));

    //actual for data_fd, set port value as register
    char ip[16]; //my ip
    unsigned int port; //my port
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    //get source IP and port from socket -- for set ip:port to camera registers
    rc = getsockname(data_fd, (struct sockaddr *)&addr, &addr_len); 
    inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
    port = ntohs(addr.sin_port);
    fprintf(stdout, "Local stream socket address %s:%u\n", ip, port);
    fflush(stdout);

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
    while(cnt < 15) {
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

int parse_gvcp_discovery_ack_packet(struct device *dev, struct gvcp_packet *packet) {
    if (!dev || !packet) return -1;
    struct gvcp_header *header = &packet->header;
    char *data = packet->data;
    if ((ntohs(header->command) == GVCP_COMMAND_DISCOVERY_ACK) && (ntohs(header->id) == 0xffff)){
        strcpy(dev->vendor, data + GVCP_MANUFACTURER_NAME_OFFSET);
        strcpy(dev->model, data + GVCP_MODEL_NAME_OFFSET);
        strcpy(dev->serial, data + GVCP_SERIAL_NUMBER_OFFSET);
        strcpy(dev->user_id, data + GVCP_USER_DEFINED_NAME_OFFSET);
        sprintf(dev->mac, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx", 
                data[GVCP_DEVICE_MAC_ADDRESS_HIGH_OFFSET + 2],
                data[GVCP_DEVICE_MAC_ADDRESS_HIGH_OFFSET + 3],
                data[GVCP_DEVICE_MAC_ADDRESS_HIGH_OFFSET + 4],
                data[GVCP_DEVICE_MAC_ADDRESS_HIGH_OFFSET + 5],
                data[GVCP_DEVICE_MAC_ADDRESS_HIGH_OFFSET + 6],
                data[GVCP_DEVICE_MAC_ADDRESS_HIGH_OFFSET + 7]);
    }
    return 0;
}

void device_print(struct device *dev) {
    fprintf(stdout, "\t======= Device Summary ======\n");
    fprintf(stdout, "\tIP/Port:\t%s:%s\n", dev->ip, dev->port);
    fprintf(stdout, "\tVendor:\t%s\n", dev->vendor);
    fprintf(stdout, "\tModel:\t%s\n", dev->model);
    fprintf(stdout, "\tSerial:\t%s\n", dev->serial);
    fprintf(stdout, "\tUser_id:\t%s\n", dev->user_id);
    fprintf(stdout, "\tMac:\t%s\n", dev->mac);
    fflush(stdout);
}

struct device *device_add(struct devices *devs, struct sockaddr *iface_addr, struct sockaddr *device_addr, struct gvcp_packet *packet) {
    struct device *dev = (struct device *)malloc(sizeof(struct device));
    if (dev == NULL) return NULL;
    memset(dev, 0, sizeof(struct device));
    dev->fd = -1;
    if (dev) {
        dev->iface_addr = (struct sockaddr *)malloc(sizeof(struct sockaddr));
        memcpy(dev->iface_addr, iface_addr, sizeof(struct sockaddr));
    }
    int rc = getnameinfo(device_addr, sizeof(struct sockaddr), dev->ip, sizeof(dev->ip), dev->port, sizeof(dev->port), NI_NUMERICHOST | NI_NUMERICSERV);
    if (rc != 0) {
        fprintf(stderr, "can't extract device addr\n");
        fflush(stderr);
        strcpy(dev->ip, "0.0.0.0");
        strcpy(dev->port, "0");
    }
    parse_gvcp_discovery_ack_packet(dev, packet);
    dev->fd = -1; //connect_to_device(dev);
    if (devs) {
        list_add_back(&devs->device_list, &dev->entry);
        devs->device_cnt++;
    }
    return dev;
}

void device_delete(struct devices *devs, struct device *dev){
    if (devs) {
        list_remove_elem(&devs->device_list, &dev->entry);
        devs->device_cnt--;
    }
    close(dev->fd);
    free(dev->iface_addr);
    free(dev);
}

struct gvcp_packet *listen_discovery_answer(int fd, struct sockaddr *addr) {
    struct sockaddr device_addr;
    socklen_t device_addr_len = sizeof(struct sockaddr);
    fd_set readfd;
    int rc = 0;
    ssize_t bytes = 0;
    struct timeval tv;
    while (1) {
        FD_ZERO(&readfd);
        FD_SET(fd, &readfd);
        memset(&tv, 0, sizeof(struct timeval));
        rc = select(fd + 1, &readfd, NULL, NULL, &tv);
        if (rc > 0) {
            if (FD_ISSET(fd, &readfd)) {
                struct gvcp_packet *packet = (struct gvcp_packet*)malloc(DEVICE_BUFFER_SIZE);
                bytes = recvfrom(fd, packet, DEVICE_BUFFER_SIZE, 0, &device_addr, &device_addr_len);
                if (bytes < 0) {
                    fprintf(stdout, "[listen_discovery_answer]: no data on inteface\n");
                    fflush(stdout);
                    free(packet);
                    return NULL;
                }
                if (bytes < sizeof(struct gvcp_header)) {
                    fprintf(stdout, "[listen_discovery_answer]: wrong on inteface\n");
                    fflush(stdout);
                    free(packet);
                    continue;
                }
                if (addr) memcpy(addr, &device_addr, sizeof(struct sockaddr));
                return packet;
            }
        }
        else {
            return NULL;
        }
    }
    return NULL;
}

struct interface *interface_add(struct interfaces *ifaces, char *name, struct sockaddr *addr) {
    struct interface *iface = (struct interface *)malloc(sizeof(struct interface));
    if (iface == NULL) return NULL;
    memset(iface, 0, sizeof(struct interface));
    iface->fd = -1;
    if (addr) {
        iface->addr = (struct sockaddr *)malloc(sizeof(struct sockaddr));
        memcpy(iface->addr, addr, sizeof(struct sockaddr));
    }
    if (name) strcpy(iface->name, name);
    iface->fd = connect_to_interface_for_discovery(iface);
    if (ifaces) {
        list_add_back(&ifaces->interface_list, &iface->entry);
        ifaces->interface_cnt++;
    }
    return iface;
}

void interface_delete(struct interfaces *ifaces, struct interface *iface) {
    if (ifaces) {
        list_remove_elem(&ifaces->interface_list, &iface->entry);
        ifaces->interface_cnt--;
    }
    close(iface->fd);
    free(iface->addr);
    free(iface);
}

//ATTENTION: memory for interfaces_list should be allocated before init
int ifaces_init(struct interfaces *ifaces) {
    memset(ifaces, 0, sizeof(struct interfaces));
    list_init(&ifaces->interface_list);
    return 0;
}

void ifaces_destroy(struct interfaces *ifaces){
    list_t *item;
    struct interface *iface;

    while (!list_is_empty(&ifaces->interface_list)) {
        item = list_first_elem(&ifaces->interface_list);
        iface = list_entry(item, struct interface, entry);
        interface_delete(ifaces, iface);
    }
}

//ATTENTION: memory for devices should be allocated before init
int devices_init(struct devices *devs){
    memset(devs, 0, sizeof(struct devices));
    list_init(&devs->device_list);
    return 0;
}

void devices_destroy(struct devices *devs){
    list_t *item;
    struct device *dev;

    while(!list_is_empty(&devs->device_list)){
        item = list_first_elem(&devs->device_list);
        dev = list_entry(item, struct device, entry);
        device_delete(devs, dev);
    }
}

int main(int argc, char *argv[]){
    struct ifaddrs *ifap = NULL;
    struct ifaddrs *ifap_iter;

    struct interfaces *ifaces = (struct interfaces *)malloc(sizeof(struct interfaces));
    ifaces_init(ifaces);

    if (getifaddrs (&ifap) < 0) {
        fprintf(stderr, "[find_devices]: can't getifaddrs\n");
        fflush(stdout);
        return -1;
    }

    for (ifap_iter = ifap; ifap_iter != NULL; ifap_iter = ifap_iter->ifa_next) {
        if ((ifap_iter->ifa_flags & IFF_UP) != 0 &&
            (ifap_iter->ifa_flags & IFF_POINTOPOINT) == 0 &&
            (ifap_iter->ifa_addr != NULL) &&
            (ifap_iter->ifa_addr->sa_family == AF_INET)) {
            struct interface *iface = interface_add(ifaces, ifap_iter->ifa_name, ifap_iter->ifa_addr);
            interface_print(iface);
        }
    }

    freeifaddrs (ifap);

    sleep(1); //may be TIMEOUT const???

    struct devices *devs = (struct devices *)malloc(sizeof(struct devices));
    struct sockaddr device_addr;
    devices_init(devs);
    list_t *elem = list_first_elem(&ifaces->interface_list);
    while(list_is_valid_elem(&ifaces->interface_list, elem)) {
        struct interface *iface = list_entry(elem, struct interface, entry);
        elem = elem->next;
        struct gvcp_packet *packet = NULL;
        while ((packet = listen_discovery_answer(iface->fd, &device_addr)) != NULL) {
            struct device *dev = device_add(devs, iface->addr, &device_addr, packet);
            interface_print(iface);
            device_print(dev);
        }
        free(packet);
    }
    ifaces_destroy(ifaces);
    free(ifaces);

    //Work with devices
    elem = list_first_elem(&devs->device_list);
    struct device *dev = list_entry(elem, struct device, entry);

    connect_to_device(dev);

    devices_destroy(devs);
    free(devs);
    return 0;
}



