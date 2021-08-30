#ifndef _MESSENGER_H_
#define _MESSENGER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>

#include "misc/list.h"
#include "misc/ring_buffer.h"

#include <netdb.h>


#define MAX_MSG_LEN 512

enum MSG_TYPE {
    MSG_UNKNOWN = -1,
    MSG_REQUEST = 0,
    MSG_REPLY = 1
};

enum MSG_CMD {
    MSG_CMD_QUIT,
    MSG_CMD_STATUS,
    MSG_CMD_SET,
    MSG_CMD_GET,
    MSG_CMD_START,
    MSG_CMD_STOP,
    MSG_CMD_CONFIG,
    MSG_CMD_CONNECT
};

struct message_actions {
    void (*start_action)(void *internal_data);
    void (*stop_action)(void *internal_data);
    void (*set_action)(void *internal_data, void *values);
    void (*get_action)(void *internal_data, void *values);
    void (*status_action)(void *internal_data, void *values);
};

struct message {
    enum MSG_CMD cmd;
    enum MSG_TYPE type;
    int src_id;
    int dst_id;
    size_t msg_len;
    char msg_string[MAX_MSG_LEN];
    struct message_actions *msg_actions;
};

int msg_get_connect_information(struct message *msg, char **phost, char **pport);

struct connection {
    list_t entry;
    int broken;
    char host[NI_MAXHOST];
    char service[NI_MAXSERV];
    int fd; //this connection file descriptor
    int epoll_fd; //-1 if not in epoll set
    size_t buffer_size;
    size_t offset;
    char buffer[MAX_MSG_LEN + 1];
};

int connection_extract_message(struct connection *conn, struct message *msg);

struct client {
    list_t connection_list;
    int connection_cnt;
    
    int epoll_fd;
    int max_events;
    pthread_t epoll_thread_id;
    int exit_flag;
    void (*sigusr2_handler)(int sig);

    struct message *(*read_action_callback)(void *internal_data);

    //struct ring_buffer *requests;
    struct ring_buffer *replies;
    void (*event_callback)(struct connection *conn, struct message *msg);
};

struct client *client_create(void (*event_callback)(struct connection *conn, struct message *msg));
void *tcp_client_process(void *arg); //@arg is pointer to 'struct client'
void client_destroy(struct client *cli);

struct server {
    list_t connection_list;
    int connection_cnt;

    char port[8];  //listen server port
    int listen_fd; //this server file descriptor
    pthread_t listen_thread_id;

    int epoll_fd;
    int max_events;

    int exit_flag;

    struct ring_buffer *requests;
    void (*execute_callback)(void *internal_data, struct message *msg);
    pthread_t execute_command_thread_id;
    pthread_mutex_t mutex_exec;
    pthread_cond_t cond_exec;
    
    struct ring_buffer *replies;
    void (*send_replies_callback)(void *internal_data, struct connection *conn);
    pthread_t send_replies_thread_id;
    pthread_mutex_t mutex_send;
    pthread_cond_t cond_send;
};

struct server *server_create(char *port, int max_events, void (*execute_callback)(void *internal_data, struct message *msg), void (*send_replies_callback)(void *internal_data, struct connection *conn));
void *tcp_server_process(void *arg); //@arg is pointer to 'struct server'
void server_destroy(struct server *srv);

struct proxy {
    int max_clients;
    struct server *srv;      //only one server in proxy supported
    int clients_cnt;
    struct clients **clients; //all clients get all messages from proxy-server requests/replies buffers by command 'status'
    void *internal_data;
};

struct proxy *proxy_create(int max_clients);
void proxy_add_server(struct proxy *px, struct server *srv);
void proxy_add_client(struct proxy *px, struct client *cli);
void proxy_destroy(struct proxy *px);

#ifdef __cplusplus
}
#endif

#endif// _MESSENGER_H_
