#ifndef _INTERFACE_H_
#define _INTERFACE_H_

#ifdef __cplusplus
extern "C" {
#endif

struct interface {
    int fd;
    char name[80];
    struct sockaddr *addr;
    list_t entry;
};


struct interfaces {
    int interface_cnt;
    list_t interface_list;
};


struct interfaces *prepare_interfaces_list();
int send_broadcast_discovery(struct interface *iface);

void interface_print(struct interface *iface);

//add discovered interface to the interfaces list
struct interface *interface_add(struct interfaces *ifaces, char *name, struct sockaddr *addr);
void interface_delete(struct interfaces *ifaces, struct interface *iface);

//ATTENTION: memory for interfaces_list should be allocated before init
int ifaces_init(struct interfaces *ifaces);
void ifaces_destroy(struct interfaces *ifaces);

#ifdef __cplusplus
}
#endif

#endif //_INTERFACE_H_

