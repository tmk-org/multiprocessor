#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <ifaddrs.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <unistd.h>

#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "capture/list.h"

//https://rsdn.org/article/unix/sockets.xml

/***************************************************/
/* sudo /sbin/ifconfig enp1s0:3 169.254.3.100/24 up
/* sudo /sbin/ifconfig enp1s0:2 169.254.2.100/24 up
/* bin/arvsource enp1s0:2 test:2  --- fake-1
/* bin/arvclient 169.254.2.100
/****************************************************/

//_discover
//gvcp_gv_discover_socket_list_new
//gvcp_enumerate_network_interfaces

//magic const from aravis
#define SOCKET_DISCOVER_BUFFER_SIZE 256*1024
#define SOCKET_BUFFER_SIZE          1024

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

//arvgvcpprivate.h / arvgvcp.c
enum gvcp_command{
    COMMAND_DISCOVERY_CMD = 0x0002,
    COMMAND_DISCOVERY_ACK = 0x0003
};

enum gvcp_packet_type{
    PACKET_TYPE_ACK = 0x00,
    PACKET_TYPE_CMD = 0x42,
    PACKET_TYPE_ERROR = 0x80,
    PACKET_TYPE_UNKNOWN_ERROR = 0x8f
};

enum gvcp_cmd_packet_flags {
    CMD_PACKET_FLAGS_NONE = 0x00,
    CMD_PACKET_FLAGS_ACK_REQUIRED = 0x01,
    CMD_PACKET_FLAGS_EXTENDED_IDS = 0x10
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

struct interface {
    int fd;
    char name[80];
    struct sockaddr *addr;
    list_t entry;
};

struct device {
    int fd;

    char ip[16];
    char port[8];

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

struct udp_header {
    int src_port;
    int dest_port;
    int length;
    int checksum;
};

struct gvcp_packet *prepare_discovery_packet(size_t *packet_size) {
    *packet_size = sizeof(struct gvcp_header);
    struct gvcp_packet *packet = (struct gvcp_packet *)malloc(*packet_size); 
    //magic from aravis (see wireshark): protocol GVCP(GigE Vision Communication Protocol) on port 3956
    packet->header.packet_type = PACKET_TYPE_CMD;
    packet->header.packet_flags = CMD_PACKET_FLAGS_ACK_REQUIRED;
    packet->header.command = htons(COMMAND_DISCOVERY_CMD);
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
    opt = SOCKET_BUFFER_SIZE;
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

int connect_to_device(struct device *dev) {
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
    device_addr.sin_addr.s_addr = inet_addr(dev->ip);;
    device_addr.sin_port = htons(atoi(dev->port));

    rc = bind(fd, dev->iface_addr, sizeof(struct sockaddr_in));
    if (rc < 0) {
        fprintf(stdout, "bind error\n");
        fflush(stdout);
        return -1;
    }

    opt = SOCKET_BUFFER_SIZE;
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

int parse_gvcp_discovery_ack_packet(struct device *dev, struct gvcp_packet *packet) {
    if (!dev || !packet) return -1;
    struct gvcp_header *header = &packet->header;
    char *data = packet->data;
    if ((ntohs(header->command) == COMMAND_DISCOVERY_ACK) && (ntohs(header->id) == 0xffff)){
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
    dev->fd = connect_to_device(dev);
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
                struct gvcp_packet *packet = (struct gvcp_packet*)malloc(SOCKET_BUFFER_SIZE);
                bytes = recvfrom(fd, packet, SOCKET_BUFFER_SIZE, 0, &device_addr, &device_addr_len);
                if (bytes < 0) {
                    fprintf(stdout, "[listen_discovery_answer]: no data on inteface\n");
                    fflush(stdout);
                    return NULL;
                }
                if (bytes < sizeof(struct gvcp_header)) {
                    fprintf(stdout, "[listen_discovery_answer]: wrong on inteface\n");
                    fflush(stdout);
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
        struct gvcp_packet *packet = listen_discovery_answer(iface->fd, &device_addr);
        if (packet) {
            struct device *dev = device_add(devs, iface->addr, &device_addr, packet);
            interface_print(iface);
            device_print(dev);
        }
        free(packet);
    }
    ifaces_destroy(ifaces);

    //Work with devices

    devices_destroy(devs);
    return 0;
}



