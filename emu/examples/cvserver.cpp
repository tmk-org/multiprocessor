#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>

#include <pthread.h>

#define MAX_PENDING_CONNECT 10

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

#include "filter/cvparams.h"


static char data[TEST_DATA_CNT][TEST_DATA_SIZE];
static int curr = -1;

void tcp_test_data_create() {
    int i, j;
    size_t data_size = TEST_DATA_SIZE;
    for (i = 0; i < TEST_DATA_CNT; i++) {
        char *chunk = (char*)((char*)data + i * TEST_DATA_SIZE); 
        //*((size_t*)chunk) = TEST_DATA_SIZE;
        //full buffer size and frame_id
        //TODO: check that TEST_HEADER_SIZE == sizeof(size_t) + sizeof(int)
        memcpy((void*)chunk, (void*)&data_size, sizeof(size_t));
        memcpy((void*)(chunk + sizeof(size_t)), (void*)&i, sizeof(int));
        char name[256];
        sprintf(name, "%s/%d.%s", getenv(TEST_SOURCE_PATH_ENV), i, "jpg");
        cv::Mat orig = cv::imread(name, cv::IMREAD_ANYDEPTH |
                                                  cv::IMREAD_ANYCOLOR);
        int origSize = orig.total() * orig.elemSize();
        if (origSize != TEST_RAW_SIZE) {
            fprintf(stdout, "[cvserver]: problems with datasize, orig = %zd, TEST_RAW_SIZE = %zd\n", origSize, TEST_RAW_SIZE);
        }
        memcpy(chunk + TEST_HEADER_SIZE, (char*)orig.data, TEST_RAW_SIZE);
        fprintf(stdout, "[cvserver]: chunk_size = %zd data_id = %d\n", *((size_t*)chunk), *((int*)(chunk + sizeof(size_t))));
    }
}

void* tcp_next_image() {
    curr++;
    if (curr % TEST_DATA_CNT == 0) curr = 0; 
    return (void*)((char*)data + (curr % TEST_DATA_CNT) * TEST_DATA_SIZE);
}

void* generate_data_thread(void *arg) {
    pthread_detach(pthread_self());
    tcp_test_data_create();
    int *fd = (int*)arg;
    int ln = -1;
    void *buf = NULL;
    size_t buf_size = 0; 
    while (1) {
        buf = (char*)tcp_next_image();  //important (char*)
        buf_size = *((size_t*)buf);
        fprintf(stdout, "[cvserver]: chunk_size = %zd\n", buf_size);
        ln = write(*fd, buf, buf_size);
        if ((ln < 0) || (ln != buf_size)) {
            break;
        }
        usleep(62500); // 62500mks = 1/16s
    }
    close(*fd);
    return NULL;
}

int tcp_stream_data_process(char *port){

    int fd;

    struct addrinfo	hints;
    struct addrinfo	*result, *rp;
    int rc, reuse_addr;
    
    int exit_flag = 0;
    
    //setenv(EXIT_FLAG_ENV, "0");
  
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
    
    pthread_t thread_id;

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
            return -1;
        }
        
        rc = getnameinfo((struct sockaddr *) &peer_addr,
		                        peer_addr_len, host, NI_MAXHOST,
		                        service, NI_MAXSERV, NI_NUMERICSERV);
		if (rc == 0) {
		    fprintf(stdout, "[tcp_stream_data_process]: Received connection from %s:%s\n", host, service);
		    fflush(stdout);
		    pthread_create(&thread_id, NULL, generate_data_thread, &acc_fd);
		    
		}
		else {
		    fprintf(stderr, "[tcp_stream_data_process]: getnameinfo %s\n", gai_strerror(rc));
            fflush(stderr);
		}
#if 0
        char buf[10];
        int bytes = read(fd, buf, sizeof(buf));
	    if (bytes <= 0){
			if (bytes < 0){
		        fprintf(stdout, "read error: %s\n", strerror(errno));
		        fflush(stdout);
		    }
			continue;
	    }  
	    else {
	        exit_flag = 1;
	    } 
#endif	       
    }
    close(fd);
    return 0;
}

int main(int argc, char *argv[]){
    tcp_stream_data_process(argv[1]);
    return 0;
}



