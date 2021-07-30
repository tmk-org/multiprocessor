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
#include "capture/interface.h"
#include "capture/device.h"
#include "capture/gvcp.h"

int get_net_parameters_from_socket(int fd, uint32_t *ip_value, uint32_t *port_value) {
    int rc = 0;
    //actual for data_fd, set ip and port value as register
    char ip[16]; //my ip
    unsigned int port; //my port
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    //get source IP and port from socket -- for set ip:port to camera registers
    rc = getsockname(fd, (struct sockaddr *)&addr, &addr_len); 
    if (rc < 0) {
        fprintf(stdout, "[get_net_parameters_from_socket]: error with getsockname\n");
        fflush(stdout);
        return -1;
    }
    inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
    port = ntohs(addr.sin_port);
    fprintf(stdout, "[get_net_parameters_from_socket]: local stream socket %d address is %s:%u\n", fd, ip, port);
    fflush(stdout);

    *ip_value = ntohl(addr.sin_addr.s_addr);
    *port_value = port;

    return 0;
}

struct gvcp_packet *listen_discovery_answer(int fd,  struct sockaddr *addr) {
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

struct device *device_create(struct sockaddr *iface_addr, struct sockaddr *device_addr) {
    struct device *dev = (struct device *)malloc(sizeof(struct device));
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
    else {
        fprintf(stdout, "dev->ip = %d dev->port = %d\n", dev->ip, dev->port);
        fflush(stdout);
    }
    return dev;
}

struct device *device_add(struct devices *devs, struct sockaddr *iface_addr, struct sockaddr *device_addr) {
    struct device *dev = device_create(iface_addr, device_addr);
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

//id_string is ip now, we think that they are unique
struct device *device_find(char *id_string) {

    struct interfaces *ifaces = prepare_interfaces_list();

    sleep(1); //may be TIMEOUT const???

    struct device *dev = (struct device *)malloc(sizeof(struct device));
    struct sockaddr device_addr;
    list_t *elem = list_first_elem(&ifaces->interface_list);
    while(list_is_valid_elem(&ifaces->interface_list, elem)) {
        struct interface *iface = list_entry(elem, struct interface, entry);
        interface_print(iface);
        elem = elem->next;
        struct gvcp_packet *packet = NULL;
        while ((packet = listen_discovery_answer(iface->fd, &device_addr) )!= NULL) {
            struct device *dev = device_create(iface->addr, &device_addr);
            gvcp_discovery_ack(dev->vendor, dev->model, dev->serial, dev->user_id, dev->mac, packet);
            if (strcmp(dev->ip, id_string) == 0) {
                fprintf(stdout, "Found device for source %d\n", 0);
                fflush(stdout);
                device_print(dev);
                ifaces_destroy(ifaces);
                free(ifaces);
                return dev;
            }
            else {
                free(dev);
            }
        }
    }
    fprintf(stdout, "No device with id_string=\'%s\' found\n", id_string);
    fflush(stdout);
    ifaces_destroy(ifaces);
    free(ifaces);
    return NULL;
}

struct devices *devices_discovery(void *internal_data, void (*new_device_callback)(void *internal_data, struct device *dev)) {
    struct devices *devs = prepare_devices_list();

    list_t *elem = list_first_elem(&devs->device_list);
    while(list_is_valid_elem(&devs->device_list, elem)) {
        struct device *dev = list_entry(elem, struct device, entry);
        elem = elem->next;
        fprintf(stdout, "Found device\n");
        fflush(stdout);
        device_print(dev);
        if (new_device_callback) new_device_callback(internal_data, dev);
    }

    return devs;
}

struct devices *prepare_devices_list() {

    struct interfaces *ifaces = prepare_interfaces_list();

    sleep(1); //may be TIMEOUT const???

    struct devices *devs = (struct devices *)malloc(sizeof(struct devices));
    struct sockaddr device_addr;
    devices_init(devs);
    list_t *elem = list_first_elem(&ifaces->interface_list);
    while(list_is_valid_elem(&ifaces->interface_list, elem)) {
        struct interface *iface = list_entry(elem, struct interface, entry);
        interface_print(iface);
        elem = elem->next;
        struct gvcp_packet *packet = NULL;
        while ((packet = listen_discovery_answer(iface->fd, &device_addr) )!= NULL) {
            struct device *dev = device_add(devs, iface->addr, &device_addr);
            gvcp_discovery_ack(dev->vendor, dev->model, dev->serial, dev->user_id, dev->mac, packet);
            device_print(dev);
        }
    }
    ifaces_destroy(ifaces);
    free(ifaces);
    return devs;
}


