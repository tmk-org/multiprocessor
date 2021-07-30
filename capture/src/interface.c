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

#include "capture/gvcp.h"

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
        fprintf(stdout, "[interface_print]: inet_ntop error\n");
    }
    fflush(stdout);
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

int send_broadcast_discovery(struct interface *iface) {
    int fd, rc = 0, opt = 1;
    struct sockaddr_in device_addr;

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

    size_t packet_size;
    struct gvcp_packet *packet = gvcp_discovery_packet(&packet_size);
    rc = sendto(fd, packet, sizeof(struct gvcp_packet), 0, (struct sockaddr*)&device_addr, sizeof(struct sockaddr_in));
    free(packet);

    return fd;
}

struct interfaces *prepare_interfaces_list() {
    struct ifaddrs *ifap = NULL;
    struct ifaddrs *ifap_iter;

    struct interfaces *ifaces = (struct interfaces *)malloc(sizeof(struct interfaces));
    ifaces_init(ifaces);

    if (getifaddrs (&ifap) < 0) {
        fprintf(stderr, "[prepare_interfaces_list]: can't getifaddrs\n");
        fflush(stdout);
        return -1;
    }

    for (ifap_iter = ifap; ifap_iter != NULL; ifap_iter = ifap_iter->ifa_next) {
        if ((ifap_iter->ifa_flags & IFF_UP) != 0 &&
            (ifap_iter->ifa_flags & IFF_POINTOPOINT) == 0 &&
            (ifap_iter->ifa_addr != NULL) &&
            (ifap_iter->ifa_addr->sa_family == AF_INET)) {
            struct interface *iface = interface_add(ifaces, ifap_iter->ifa_name, ifap_iter->ifa_addr);
            iface->fd = send_broadcast_discovery(iface);
            interface_print(iface);
        }
    }

    freeifaddrs (ifap);
    return ifaces;
}
