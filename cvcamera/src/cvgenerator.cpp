#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>

#include <sys/uio.h>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

#include <semaphore.h>
#include <pthread.h>

#include "cvcamera/cvcamera.h"
#include "cvcamera/cvgenerator.h"

#define MAX_PENDING_CONNECT 10

struct data_generator {
    int fd;
    int exit_flag;
    pthread_t thread_id;
    void *data;
    int curr;
};

void test_data_create(void *data) {
    int i, j;
    size_t data_size = DATA_SIZE;
    for (i = 0; i < DATA_CNT; i++) {
        char *chunk = (char*)((char*)data + i * DATA_SIZE); 
        //*((size_t*)chunk) = DATA_SIZE;
        //full buffer size and frame_id
        //TODO: check that IMAGE_HEADER_SIZE == sizeof(size_t) + sizeof(int)
        memcpy((void*)chunk, (void*)&data_size, sizeof(size_t));
        memcpy((void*)(chunk + sizeof(size_t)), (void*)&i, sizeof(int));
        char name[256];
        sprintf(name, "%s/%d.%s", getenv(SOURCE_PATH_ENV), i, "jpg");
        cv::Mat orig = cv::imread(name, cv::IMREAD_ANYDEPTH |
                                                  cv::IMREAD_ANYCOLOR);
        int origSize = orig.total() * orig.elemSize();
        if (origSize != IMAGE_RAW_SIZE) {
            fprintf(stdout, "[cvserver]: problems with datasize, orig = %d, IMAGE_RAW_SIZE = %zd\n", origSize, (size_t)IMAGE_RAW_SIZE);
        }
        memcpy(chunk + IMAGE_HEADER_SIZE, (char*)orig.data, IMAGE_RAW_SIZE);
        fprintf(stdout, "[cvserver]: chunk_size = %zd data_id = %d\n", *((size_t*)chunk), *((int*)(chunk + sizeof(size_t))));
    }
}

void* next_image(struct data_generator *generator) {
    int curr = generator->curr++;
    if (curr % DATA_CNT == 0) curr = 0; 
    return (void*)((char*)(generator->data) + (curr % DATA_CNT) * DATA_SIZE);
}

void* generate_data_thread(void *arg) {
    pthread_detach(pthread_self());
    struct data_generator *generator = (struct data_generator *)arg;
    int fd = generator->fd;
    int exit_flag = generator->exit_flag;
    int ln = -1;
    void *buf = NULL;
    size_t buf_size = 0;
    while (!exit_flag) {
        buf = (char*)next_image(generator);  //important (char*)
        buf_size = *((size_t*)buf);
        fprintf(stdout, "[generate_data_thread]: chunk_size = %zd\n", buf_size);
        ln = write(fd, buf, buf_size);
        if ((ln < 0) || (ln != buf_size)) {
            break;
        }
        usleep(62500); // 62500mks = 1/16s
    }
    close(fd);
    return NULL;
}

struct data_generator *generator_init(char *port){

    char *data = (char*)malloc(DATA_CNT * DATA_SIZE * sizeof(char));

    struct data_generator *generator = (struct data_generator *)malloc(sizeof(struct data_generator));
    generator->data = (void*)data;
    generator->curr = -1;
    generator->exit_flag = 0;

    fprintf(stdout, "[generator]: prepare data from %s\n", getenv(SOURCE_PATH_ENV));
    fflush(stdout);

    test_data_create(generator->data);

    fprintf(stdout, "[generator]: data ready, waitting for connection...\n");
    fflush(stdout);

    struct addrinfo	hints;
    struct addrinfo	*result, *rp;
    int rc, reuse_addr;

    int exit_flag = 0;

    int fd;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    rc = getaddrinfo(NULL, port, &hints, &result);
    if (rc != 0) {
	    fprintf(stderr, "[tcp_stream_data_process]: getaddrinfo: %s\n", gai_strerror(rc));
	    fflush(stderr);
	    exit(EXIT_FAILURE);
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
	    fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
	    if (fd == -1) continue;

	    reuse_addr=1;
	    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr)) != 0) {
	        close(fd);
	        continue;
	    }

	    if (bind(fd, rp->ai_addr, rp->ai_addrlen) == 0) break;
	    close(fd);
    }

    if (rp == NULL) {
	fprintf(stderr, "[tcp_stream_data_process]: Could not bind\n");
	fflush(stderr);
	exit(EXIT_FAILURE);
    }

    freeaddrinfo(result);

    if (listen(fd, MAX_PENDING_CONNECT) != 0){
	fprintf(stderr, "[tcp_stream_data_process]: Could not listen, error: %s\n", strerror(errno));
	fflush(stderr);
	exit(EXIT_FAILURE);
    }

    while(!exit_flag){
	fd_set	read_fds;
	int	rc, max;

	FD_ZERO(&read_fds);

	max = fd;
	FD_SET(fd, &read_fds);

	rc = select(max + 1, &read_fds, NULL, NULL, NULL);
	if (rc == -1){
	    fprintf(stderr, "[tcp_stream_data_process]: Could not select, error: %s\n", strerror(errno));
	    fflush(stderr);
	    exit(EXIT_FAILURE);
	}
	else if (rc == 0) { //timeout
	    continue;
	}

        if (!FD_ISSET(fd, &read_fds)) {
            fprintf(stderr, "[tcp_stream_data_process]: Socket file descriptor is not set in read_fds, error: %s\n", strerror(errno));
	        fflush(stderr);
            continue;
        }

	// new connection
        struct sockaddr_storage	peer_addr;
	socklen_t peer_addr_len;
	char host[NI_MAXHOST], service[NI_MAXSERV];
        peer_addr_len = sizeof(peer_addr);
	int acc_fd = accept(fd, (struct sockaddr *) &peer_addr, &peer_addr_len);
        if (acc_fd == -1) {
            //Non-fatal errors
            if ((errno == ECONNABORTED) || (errno == EINTR) ||
                (errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                continue;
            }
            fprintf(stderr, "[tcp_stream_data_process]:  Accept failed, error: %s\n", strerror(errno));
	    fflush(stderr);
            close(fd);
            return NULL;
        }

        rc = getnameinfo((struct sockaddr *) &peer_addr,
		                        peer_addr_len, host, NI_MAXHOST,
		                        service, NI_MAXSERV, NI_NUMERICSERV);
		if (rc == 0) {
		    fprintf(stdout, "[tcp_stream_data_process]: Received connection from %s:%s\n", host, service);
		    fflush(stdout);
		    generator->fd = acc_fd;
		    pthread_create(&generator->thread_id, NULL, generate_data_thread, generator);
		}
		else {
		    fprintf(stderr, "[tcp_stream_data_process]: getnameinfo %s\n", gai_strerror(rc));
            fflush(stderr);
		}
    }
    close(fd);
    return generator;
}

void generator_destroy(struct data_generator *generator){
    free(generator->data);
    generator->exit_flag = 1;
    pthread_join(generator->thread_id, NULL);
    close(generator->fd);
    free(generator);
}



