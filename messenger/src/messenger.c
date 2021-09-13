#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

//usage strcasestr()
#define _GNU_SOURCE
    #include <string.h> 

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>

#include "misc/list.h"
#include "misc/ring_buffer.h"

#include "messenger/messenger.h"

struct connection *connection_add(list_t *connection_list, int *connection_cnt, int fd){
    struct connection *connection = (struct connection *)malloc(sizeof(struct connection));
    if (connection == NULL) return NULL;
    memset(connection, 0, sizeof(struct connection));
    connection->fd = fd;
    if (connection_list) {
        list_add_back(connection_list, &connection->entry);
        (*connection_cnt)++;
    }
    return connection;
}

void connection_delete(list_t *connection_list, int *connection_cnt, struct connection *connection){
    if (connection_list) {
        list_remove_elem(connection_list, &connection->entry);
        (*connection_cnt)--;
    }
    close(connection->fd);
    free(connection);
}

struct connection *connection_get(char *host, char *port, list_t *connection_list){
    list_t *item;
    struct connection *connection;
    item = list_first_elem(connection_list);
    while(list_is_valid_elem(connection_list, item)){
        connection = list_entry(item, struct connection, entry);
        fprintf(stdout, "[connection_get]: compare '%s':'%s' vs '%s':'%s'\n", connection->host, connection->service, host, port);
        fflush(stdout);
        if (strcmp(connection->service, port) == 0 && strcmp(connection->host, host) == 0) {
            return connection;
        }
        item = item->next;
    }
    return NULL;
}

//return 0 if add_connection is ok, else return 1
int server_add_connection(char *host, char *service, struct server *srv, int fd) {
    struct epoll_event ev;
    
    struct connection *new_conn = connection_add(&srv->connection_list, &srv->connection_cnt, fd);
    if (new_conn == NULL) {
        fprintf(stderr, "[server_add_connection]: couldn't add new connection for fd = %d\n", fd);
        fflush(stderr);
        close(fd);
        return 1;
    }

    strncpy(new_conn->host, host, sizeof(new_conn->host));
    strncpy(new_conn->service, service, sizeof(new_conn->service));

    // add new connection to the listen on epoll
    ev.events = EPOLLIN | EPOLLRDHUP;
    ev.data.ptr = new_conn;
    if (epoll_ctl(srv->epoll_fd, EPOLL_CTL_ADD, fd, &ev) != 0){
        fprintf(stderr, "[server_add_connection]: could not add file descriptor to epoll set, error: %s\n", strerror(errno));
        fflush(stderr);
        connection_delete(&srv->connection_list, &srv->connection_cnt, new_conn);
        free(new_conn);
        return 1;
    }

    return 0;
}

//return 0 if add_connection is ok, else return -1 (critical error)
int client_add_connection(char *host, char *port, struct client *cli) {
    struct epoll_event ev;

    int fd;
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int rc;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;     //IPv4 && IPv6
    hints.ai_socktype = SOCK_STREAM; //TCP socket
    hints.ai_flags = 0;
    hints.ai_protocol = 0;           //Any protocol

    rc = getaddrinfo(host, port, &hints, &result);
    if (rc != 0) {
        fprintf(stderr, "[client_add_connection]: getaddrinfo error %s\n", gai_strerror(rc));
        fflush(stderr);
        return -1;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd == -1) continue;
        if (connect(fd, rp->ai_addr, rp->ai_addrlen) != -1) break;
        close(fd);
    }

    if (rp == NULL) {
        fprintf(stderr, "[client_add_connection]: could not connect\n");
        fflush(stderr);
        return -1;
    }

    freeaddrinfo(result);

    struct connection *new_conn = connection_add(&cli->connection_list, &cli->connection_cnt, fd);
    if (new_conn == NULL) {
        fprintf(stderr, "[client_add_connection]: couldn't add new connection for fd = %d\n", fd);
        fflush(stderr);
        close(fd);
        return 1;
    }

    strncpy(new_conn->host, host, sizeof(new_conn->host));
    strncpy(new_conn->service, port, sizeof(new_conn->service));
    new_conn->fd = fd;

    // add new connection to the listen on epoll
    ev.events = EPOLLIN | EPOLLRDHUP;
    ev.data.ptr = new_conn;
    if (epoll_ctl(cli->epoll_fd, EPOLL_CTL_ADD, fd, &ev) != 0){
        fprintf(stderr, "[client_add_connection]: could not add fd = %d to epoll set, error: %s\n", fd, strerror(errno));
        fflush(stderr);
        connection_delete(&cli->connection_list, &cli->connection_cnt, new_conn);
        free(new_conn);
        return -1;
    }

    return fd;
}

int read_connection_data(struct connection *conn) {
    if (!conn) {
        return 1;
    }
    //get new data
    ssize_t bytes;
    int remainder;
    remainder = sizeof(conn->buffer) - conn->offset;
    if (remainder <= 0) {
        //connection_mark_broken(&srv, connection);
        return 1;
    }
    bytes = read(conn->fd, conn->buffer + conn->offset, remainder);
    if (bytes <= 0){
        //connection_mark_broken(&srv, connection);
        if (bytes < 0){
            fprintf(stdout, "[read_connection_data, fd = %d]: read error %s\n", conn->fd, strerror(errno));
            fflush(stdout);
        }
        return 1;
    }
    conn->offset += bytes;
    return 0;
}

struct message *default_read_action(void* internal_data) {
    (void)internal_data;
    struct message *msg = (struct message *)malloc(sizeof(struct message));
    fprintf(stdout, "[default_read_action]: we are IN the client command reader\n");
    fflush(stdout);
    fprintf(stdout, "$ ");
    fflush(stdout);
    scanf("%[^\n]%*c", msg->msg_string);
    msg->type = MSG_UNKNOWN;
    strcat(msg->msg_string, "\n");
    msg->msg_len = strlen(msg->msg_string);
    return msg;
}

void parse_api_request(struct message *msg) {
    if (!msg || msg->msg_len == 0) {
        fprintf(stdout, "[parse_api_request]: no message data\n");
        fflush(stdout);
        return;
    }
    char *buf = msg->msg_string;
    msg->type = MSG_REQUEST;
    if (strcasestr(buf, "RE") != 0 || strcasestr(buf, "ER") != 0 ) {
        fprintf(stdout, "[parse_api_request]: reply message recieved\n");
        fflush(stdout);
        msg->type = MSG_REPLY;
        msg->cmd_len = 2;
    }
    if (strcasestr(buf, "CONNECT") != 0) {
        fprintf(stdout, "[parse_api_request]: connect message recieved\n");
        fflush(stdout);
        msg->cmd = MSG_CMD_CONNECT;
        msg->cmd_len = strlen("CONNECT");
    }
    if (strcasestr(buf, "START") != 0) {
        fprintf(stdout, "[parse_api_request]: start message recieved\n");
        fflush(stdout);
        msg->cmd = MSG_CMD_START;
        msg->cmd_len = strlen("START");
    }
    if (strcasestr(buf, "STOP") != 0) {
        fprintf(stdout, "[parse_api_request]: stop message recieved\n");
        fflush(stdout);
        msg->cmd = MSG_CMD_STOP;
        msg->cmd_len = strlen("STOP");
    }
    if (strcasestr(buf, "STATUS") != 0) {
        fprintf(stdout, "[parse_api_request]: status message recieved\n");
        fflush(stdout);
        msg->cmd = MSG_CMD_STATUS;
        msg->cmd_len = strlen("STATUS");
     }
     if (strcasestr(buf, "SET") != 0) {
        fprintf(stdout, "[parse_api_request]: set message recieved\n");
        fflush(stdout);
        msg->cmd = MSG_CMD_SET;
        msg->cmd_len = strlen("SET");
     }
     if (strcasestr(buf, "GET") != 0) {
        fprintf(stdout, "[parse_api_request]: get message recieved\n");
        fflush(stdout);
        msg->cmd = MSG_CMD_GET;
        msg->cmd_len = strlen("GET");
     }
     if (strcasestr(buf, "CONFIG") != 0) {
        fprintf(stdout, "[parse_api_request]: get message recieved\n");
        fflush(stdout);
        msg->cmd = MSG_CMD_CONFIG;
        msg->cmd_len = strlen("CONFIG");
     }
     if (strcasestr(buf, "QUIT") != 0 || strcasecmp(buf, "Q") == 0 || strcasecmp(buf, "Q\n") == 0) {
        fprintf(stdout, "[parse_api_request]: quit message recieved\n");
        fflush(stdout);
        msg->cmd = MSG_CMD_QUIT;
        msg->cmd_len = strlen("QUIT");
     }
}

int msg_get_connect_information(struct message *msg, char **phost, char **pport) {
    char *result = strdup(msg->msg_string + msg->cmd_len + 1);
    char *port = strrchr(result, ' ');
    if (!port) {
        fprintf(stdout, "[msg_get_connect_information]: not enouth data for connect\n");
        fflush(stdout);
        return -1;
    }
    *port = '\0';
    port++;
    *phost = result;
    port[strlen(port)-1] = '\0';
    *pport = port;
    fprintf(stdout, "[msg_get_connect_information]: connect to '%s:%s'\n", *phost, *pport);
    fflush(stdout);
    return 0;
}

//this message will be printed when SIGUSR2 signal on epoll_pwait
void sigusr2_handler(int sig){
    (void)sig;
    fprintf(stdout, "[sigusr2_handler]: sigusr2 signal\n");
    fflush(stdout);
}

void client_execute_callback(void *internal_data, struct message *msg) {
    struct client *cli = (struct client *)internal_data;
    char *host, *port;
    switch (msg->cmd) {
        case MSG_CMD_QUIT: {
            cli->exit_flag = 1;
            fprintf(stdout, "[client_execute_callback]: sending SIGUSR2 to epoll thread ...\n");
            fflush(stdout);
            pthread_kill(cli->epoll_thread_id, SIGUSR2);
            break;
        }
        case MSG_CMD_CONNECT: {
            //add new connection to listen on epoll_wait
            if (msg_get_connect_information(msg, &host, &port) == 0) {
                int fd = client_add_connection(host, port, cli);
                fprintf(stdout, "[client_execute_callback]: new connection fd = %d for command %s\n", fd, msg->msg_string);
                fflush(stdout);
            }
            else {
                fprintf(stdout, "[client_execute_callback]: connection information is wrong %s\n", msg->msg_string);
                fflush(stdout);
            }
            break;
        }
        case MSG_CMD_STATUS: {
            //TODO: read all from replies ring buffer
            fprintf(stdout, "[client_execute_callback]: command status is not implemented %s\n", msg->msg_string);
            fflush(stdout);
            break;
        }
        case MSG_CMD_STOP:
        case MSG_CMD_START:
        case MSG_CMD_SET:
        case MSG_CMD_GET: {
            if (msg_get_connect_information(msg, &host, &port) == 0) {
                struct connection *conn = connection_get(host, port, &cli->connection_list);
                int fd;
                if (!conn) {
                    fprintf(stdout, "[client_execute_callback]: wrong connection to %s:%s, try to reconnect\n", msg->msg_string, host, port);
                    fflush(stdout);
                    fd = client_add_connection(host, port, cli);
                    if (fd == -1) {
                        break;
                    }
                }
                fd = conn->fd;
                fprintf(stdout, "[client_execute_callback]: sent command %s to connection fd = %d\n", msg->msg_string, conn->fd);
                fflush(stdout);
                write(conn->fd, msg->msg_string, msg->msg_len);
            }
            else {
                fprintf(stdout, "[client_execute_callback]: connection information is wrong %s\n", msg->msg_string);
                fflush(stdout);
            }
            break;
        }
        default: 
            fprintf(stdout, "[client_execute_callback]: unsupported command %s [%d]\n", msg->msg_string, msg->cmd);
            fflush(stdout);
    }
};

void client_event_callback(void* internal_data, struct message *msg) {
    (void)internal_data;
    //struct connection *conn = (struct connection *)conn;
    fprintf(stdout, "[client_event_callback]: will push to replies queue msg = %s\n", msg->msg_string);
    fflush(stdout);
}

void *event_thread(void *arg) {
    struct client *cli = (struct client *)arg;
    //client_execute_callback((void *)conn, msg);
    struct epoll_event *events = (struct epoll_event *)malloc(cli->max_events *sizeof(struct epoll_event));
    while (!cli->exit_flag) {
        int i, timeout_ms = -1;
        //ATTENTION: we can STOP this thread ONLY with signal
#if 0
        int n = epoll_wait(cli->epoll_fd, events, cli->max_events, timeout_ms);
#else
        int n = epoll_pwait(cli->epoll_fd, events, cli->max_events, -1, &cli->sigset);
#endif

        if (cli->exit_flag) {
            fprintf(stdout, "[event_thread]: ok, stopping thread by signal (cli->exit_flag = %d)\n", cli->exit_flag);
            fflush(stdout);
            break;
        }

        if (n < 0) {
            if (errno == EAGAIN || errno == EINTR) {
                fprintf(stdout, "[event_thread]: %s on epoll_wait\n", strerror(errno));
                fflush(stdout);
                continue;
            }
            fprintf(stderr, "[event_thread]: could not wait for epoll events, error: %s\n", strerror(errno));
            fflush(stderr);
            close(cli->epoll_fd);
            return NULL;
        }

        for(i = 0; i < n; i++) {
            struct connection *conn = (struct connection*)events[i].data.ptr;
            if (read_connection_data(conn) == 1) {
                fprintf(stdout, "[event_thread]: problems with read, close connection %d\n", conn->fd);
                fflush(stdout);
                //TODO: clean all broken connections
                connection_delete(&cli->connection_list, &cli->connection_cnt, conn);
                continue;
            }
            //working with recieved data, find the end of message and parse
            struct message *msg = (struct message *)malloc(sizeof(struct message)); 
            if (connection_extract_message(conn, msg) == 1) { //make msg structure from buffer data
                if (cli->event_callback == NULL) {
                    fprintf(stdout, "[event_thread]: we are with the null server execute callback\n");
                    fflush(stdout);
                    client_event_callback((void*)conn, msg);
                }
                else { //make something with connection->request
                    fprintf(stdout, "[event_thread]: we are with the NOT null server execute callback\n");
                    fflush(stdout);
                    cli->event_callback((void*)cli, msg);
                }
            }
        }
    }
    free(events);
    return NULL;
}

//cycle on epoll_wait for translate messages to server
void *tcp_client_process(void *arg){
    struct client *cli = (struct client*)arg;

    pthread_sigmask(SIG_SETMASK, NULL, &cli->sigset);
    sigdelset(&cli->sigset, SIGUSR2);

    int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd < 0){
        fprintf(stderr, "[tcp_client_thread]: could not create epoll file descriptor: %s\n", strerror(errno));
        fflush(stderr);
        return NULL;
    }

    cli->epoll_fd = epoll_fd;
    //add thread with cycling on epoll_wait for get asycn replies
    pthread_create(&cli->epoll_thread_id, NULL, event_thread, cli);

    while (!cli->exit_flag) {
        struct message *msg = NULL;
        if (cli->read_action_callback == NULL) {
            msg = default_read_action((void*)cli);
        }
        else {
            msg = cli->read_action_callback((void*)cli);
        }
        parse_api_request(msg);
        client_execute_callback((void *)cli, msg);
    }

    fprintf(stdout, "[tcp_client_process]: sending SIGUSR2 to epoll thread ...\n");
    fflush(stdout);
    pthread_join(cli->epoll_thread_id, NULL);

    close(epoll_fd);
    return NULL;
}

struct client *client_create(void (*event_callback)(struct connection *conn, struct message *msg)) {
    struct client *cli = (struct client *)malloc(sizeof(struct client));
    memset(cli, 0, sizeof(struct client));
    //cli->state = CLIENT_STOPPED;
    list_init(&cli->connection_list);
    cli->exit_flag = 0;
    cli->max_events = 10;
    cli->event_callback = event_callback;

    sigemptyset(&cli->sigset);
    sigaddset(&cli->sigset, SIGUSR2);
    pthread_sigmask(SIG_BLOCK, &cli->sigset, NULL);
    sigemptyset(&cli->action.sa_mask);
    cli->action.sa_handler = sigusr2_handler;
    cli->action.sa_flags = 0;
    sigaction(SIGUSR2, &cli->action, NULL);

    return cli;
}

void client_destroy(struct client *cli) {
    list_t *item;
    struct connection *connection;
    //cli->state = CLIENT_STOPPED;
    while(!list_is_empty(&cli->connection_list)) {
        item = list_first_elem(&cli->connection_list);
        connection = list_entry(item, struct connection, entry);
        connection_delete(&cli->connection_list, &cli->connection_cnt, connection);
    }
    free(cli);
}

void server_execute_callback(void *internal_data, struct message *msg) {
    struct server *srv = (struct server *)internal_data;
    fprintf(stdout, "[server_execute_callback]: Hello from server_execute_callback\n");
    fflush(stdout);
    switch (msg->cmd) {
        case MSG_CMD_STOP:
            srv->exit_flag = 1;
            fprintf(stdout, "[server_execute_callback]: sending SIGUSR2 to listen thread ...\n");
            fflush(stdout);
            pthread_kill(srv->listen_thread_id, SIGUSR2);
            break;
        //case MSG_CMD_STATUS:
        //    ring_buffer_pop();
        default: 
            fprintf(stdout, "[server_execute_callback]: unsupported command %s\n", msg->msg_string);
            fflush(stdout);
    }
};

void server_send_replies_callback(void *internal_data, struct connection *conn) {
    (void)internal_data;
    char *reply = strdup("[server_send_replies_callback]: RE{ok}\n");
    write(conn->fd, reply, strlen(reply));
};

//return '1' if message found and '0' else
int connection_extract_message(struct connection *conn, struct message *msg) {
    if (!msg) {
        fprintf(stdout, "[connection_extract_message]: memory for message is NULL\n");
        fflush(stdout);
        return 0;
    }
    char *endl = memchr(conn->buffer, '\n', conn->offset);
    if (endl) {
        *endl = '\0';
        fprintf(stdout, "[connection_extract_message]: {connection %d} %s\n", conn->fd, conn->buffer);
        fflush(stdout);
        msg->type = MSG_REQUEST;
        //TODO: map from connection->service
        msg->src_id = -1; //not defined
        msg->dst_id = -1; //not defined
        msg->msg_len = strlen(conn->buffer);
        strncpy(msg->msg_string, conn->buffer, msg->msg_len);
        parse_api_request(msg);
        return 1;
    }
    endl++;
    memmove(conn->buffer, endl, sizeof(conn->buffer) - (endl - conn->buffer));
    conn->offset -= (endl - conn->buffer);
    return 0;
};

void *tcp_server_process(void *arg){
    struct server *srv = (struct server*)arg;
    int epoll_fd, listen_fd;
    struct epoll_event ev;
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int rc, reuse_addr;

    pthread_sigmask(SIG_SETMASK, NULL, &srv->sigset);
    sigdelset(&srv->sigset, SIGUSR2);
 
    epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd < 0){
        fprintf(stderr, "[tcp_server_process]: could not create epoll file descriptor: %s\n", strerror(errno));
        fflush(stderr);
        return NULL;
    }
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;     // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP socket
    hints.ai_flags = AI_PASSIVE;     // For wildcard IP address
    hints.ai_protocol = 0;           // Any protocol
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    rc = getaddrinfo(NULL, srv->port, &hints, &result);
    if (rc != 0) {
        fprintf(stderr, "[tcp_server_process]: getaddrinfo: %s\n", gai_strerror(rc));
        fflush(stderr);
        close(epoll_fd);
        return NULL;
    }

    /* getaddrinfo() returns a list of address structures.
       Try each address until we successfully bind(2).
       If socket(2) (or bind(2)) fails, we (close the socket
       and) try the next address. */

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        listen_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (listen_fd == -1) continue;
        reuse_addr = 1;
        if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr)) != 0) {
            close(listen_fd);
            break;
        }
        if (bind(listen_fd, rp->ai_addr, rp->ai_addrlen) == 0) break;
    }

    if (rp == NULL) {
        fprintf(stderr, "[tcp_server_process]: could not bind fd = %d, %s\n",
            listen_fd, strerror(errno));
        close(listen_fd);
        close(epoll_fd);
        return NULL;
    }

    freeaddrinfo(result);

    if (listen(listen_fd, 10) != 0){
        fprintf(stderr, "[tcp_listen_thread]: could not listen, error: %s\n", strerror(errno));
        fflush(stderr);
        close(listen_fd);
        close(epoll_fd);
        return NULL;
    }

    ev.events = EPOLLIN | EPOLLRDHUP;
    ev.data.ptr = &listen_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev) != 0){
        fprintf(stderr, "[tcp_server_process]: could not add file descriptor to epoll set, error: %s\n", strerror(errno));
        fflush(stderr);
        close(listen_fd);
        close(epoll_fd);
        return NULL;
    }

    srv->epoll_fd = epoll_fd;
    srv->listen_fd = listen_fd;

    struct epoll_event *events = (struct epoll_event *)malloc(srv->max_events *sizeof(struct epoll_event));
    while (!srv->exit_flag) {
        int i, timeout_ms = -1;
        //ATTENTION: we can STOP this thread ONLY with signal
#if 0
        int n = epoll_wait(epoll_fd, events, srv->max_events, timeout_ms);
#else
        int n = epoll_pwait(epoll_fd, events, srv->max_events, -1, &srv->sigset);
#endif

        if (srv->exit_flag) {
            fprintf(stdout, "[tcp_server_process]: ok, stopping thread by signal (srv->exit_flag = %d)\n", srv->exit_flag);
            fflush(stdout);
            break;
        }

        if (n < 0) {
            if (errno == EAGAIN || errno == EINTR) {
                fprintf(stdout, "[tcp_server_process]: %s on epoll_wait\n", strerror(errno));
                fflush(stdout);
                continue;
            }
            fprintf(stderr, "[tcp_server_process]: could not wait for epoll events, error: %s\n", strerror(errno));
            fflush(stderr);
            close(listen_fd);
            close(epoll_fd);
            return NULL;
        }

        for(i = 0; i < n; i++){
            if (events[i].data.ptr == &listen_fd) {
                //new event or error
                struct sockaddr_storage peer_addr;
                socklen_t peer_addr_len;
                char host[NI_MAXHOST], service[NI_MAXSERV];
                int fd;

                peer_addr_len = sizeof(peer_addr);
                fd = accept(listen_fd, (struct sockaddr *) &peer_addr, &peer_addr_len);
                if (fd < 0){
                    fprintf(stderr, "[tcp_server_process]: could not accept new connection, error: %s\n", strerror(errno));
                    fflush(stderr);
                    continue;
                }

                rc = getnameinfo((struct sockaddr *) &peer_addr,
                                peer_addr_len, host, NI_MAXHOST,
                                service, NI_MAXSERV, NI_NUMERICSERV);
                if (rc == 0) {
                    fprintf(stdout, "[tcp_server_process]: received connection from %s:%s\n", host, service);
                    fflush(stdout);
                }
                else {
                    fprintf(stderr, "[tcp_server_process]: getnameinfo: %s\n", gai_strerror(rc));
                    fflush(stdout);
                }

                if (server_add_connection(host, service, srv, fd) == 1) {
                    continue;
                }
            }
            else {
                struct connection *conn = (struct connection *)events[i].data.ptr;
                if (read_connection_data(conn) == 1) {
                    fprintf(stdout, "[tcp_server_process]: close connection %d\n", conn->fd);
                    fflush(stdout);
                    //TODO: clean all broken connections
                    connection_delete(&srv->connection_list, &srv->connection_cnt, conn);
                    continue;
                }
                //working with recieved data, find the end of message and parse
                //TODO: take from preallocated buffer
                struct message *msg = (struct message *)malloc(sizeof(struct message)); 
                if (connection_extract_message(conn, msg) == 1) { // make msg structure from buffer data
                    if (srv->execute_callback == NULL) {
                        fprintf(stdout, "[tcp_server_process]: we are with the null server execute callback\n");
                        fflush(stdout);
                        server_execute_callback((void*)srv, msg);
                    }
                    else { //make something with connection->request
                        fprintf(stdout, "[tcp_server_process]: we are with the NOT null server execute callback\n");
                        fflush(stdout);
                        srv->execute_callback((void*)srv, msg);
                    }
                }
            }
        }
    }

    close(listen_fd);
    close(epoll_fd);
    return NULL;
}

struct server *server_create(char *port, int max_events, 
                             void (*execute_callback)(void *internal_data, struct message *msg),
                             void (*send_replies_callback)(void *internal_data, struct connection *conn)){
    struct server *srv = (struct server*)malloc(sizeof(struct server));
    memset(srv, 0, sizeof(struct server));
    srv->max_events = max_events;
    srv->exit_flag = 0;
    strcpy(srv->port, port);
    srv->connection_cnt = 0;
    srv->execute_callback = execute_callback;
    srv->send_replies_callback = send_replies_callback;
    list_init(&srv->connection_list);

    sigemptyset(&srv->sigset);
    sigaddset(&srv->sigset, SIGUSR2);
    pthread_sigmask(SIG_BLOCK, &srv->sigset, NULL);
    sigemptyset(&srv->action.sa_mask);
    srv->action.sa_handler = sigusr2_handler;
    srv->action.sa_flags = 0;
    sigaction(SIGUSR2, &srv->action, NULL);

    pthread_create(&srv->listen_thread_id, NULL, tcp_server_process, srv);
    return srv;
}

void server_destroy(struct server *srv) {
    list_t *item;
    struct connection *connection;
    //srv->state = SERVER_STOPPED;
    while(!list_is_empty(&srv->connection_list)){
        item = list_first_elem(&srv->connection_list);
        connection = list_entry(item, struct connection, entry);
        connection_delete(&srv->connection_list, &srv->connection_cnt, connection);
    }
    srv->exit_flag = 1;
    //ATTENTION: epoll_wait could be stopped only by signal 
    fprintf(stdout, "[server_destroy]: sending SIGUSR2 to epoll thread ...\n");
    fflush(stdout);
    pthread_join(srv->listen_thread_id, NULL);
    free(srv);
}

struct proxy *proxy_create(int max_clients) {
    struct proxy *px = (struct proxy *)malloc(sizeof(struct proxy));
    px->max_clients = max_clients;
    px->clients_cnt = 0;
    px->clients = (struct client **)malloc(px->max_clients * sizeof(struct client *) * sizeof(struct client));
    for (int i = 0; i < px->clients_cnt; i++) {
        px->clients[i] = NULL;
    }
    return px;
}

void proxy_add_server(struct proxy *px, struct server *srv) {
    px->srv = srv;
}

void proxy_add_client(struct proxy *px, struct client *cli) {
    px->clients[px->clients_cnt] = cli;
    px->clients_cnt++;
}

void proxy_destroy(struct proxy *px) {
    for (int i = 0; i < px->clients_cnt; i++) {
        struct client *cli = px->clients[i];
        client_destroy(cli);
    }
    free(px->clients);
}



