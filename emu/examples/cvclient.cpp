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

#include "filter/cvparams.h"
#include "filter/cvfilter.h"

struct obj_collector {
    union {
        int fd;
        char addr[12];
        char port[8];
    };
    int exit_flag;
    sem_t obj_ready;
    pthread_t thread_id;
    //??? may be with mutex
    int frames_cnt;
};

void *collect_data_thread(void *arg){

    struct obj_collector *collector = (struct obj_collector*)arg;
    int fd = collector->fd;
    int *exit_flag = &collector->exit_flag;

    cv::Mat img;
    img = cv::Mat::zeros(TEST_RAW_HEIGHT, TEST_RAW_WIDTH, CV_8UC3);
    int imgSize = img.total() * img.elemSize();
    
    char header[TEST_HEADER_SIZE];
    size_t  data_size = 0;
    int frame_id = -1;
    struct iovec iov[2];
    
    iov[0].iov_base = (char*)header; //(size_t*)&data_size;
    iov[0].iov_len = TEST_HEADER_SIZE; 
    iov[1].iov_base = (char*)img.data;
    iov[1].iov_len = TEST_RAW_SIZE;

    //int iovcnt = sizeof(iov) / sizeof(struct iovec);

    ssize_t bytes = 0, recieved_len = 0, remaining_len = iov[0].iov_len;
    char name[256];
    int id = 0;

#if 0
    if (!img.isContinuous()) {
        img = img.clone();
    }
#endif

    int header_ok = 0;

    int frames_obj_cnt = 0;
    int frames_empty_cnt = 0;
    int frames_non_cnt = 0;
    int new_object = 0;

    while (!(*exit_flag)) {
        if (remaining_len == 0) {
            //fprintf(stdout, "wtf???\n");
            remaining_len = iov[0].iov_len;
            continue;
        }
#if 0
        if (!header_ok && recieved_len == 0) {
            fprintf(stdout, "1\n");
            bytes = readv(fd, iov, iovcnt);
        }
        else 
#endif
        if (!header_ok) {
            //fprintf(stdout, "2\n");
            bytes = read(fd, (char*)iov[0].iov_base + recieved_len, remaining_len);
        }
        else {
            //fprintf(stdout, "3: recv = %zd, rem = %zd\n", recieved_len, remaining_len);
            bytes = read(fd, (char*)iov[1].iov_base + recieved_len, remaining_len);
        }
        if (bytes < 0) {
            //fprintf(stdout, "[tcp_capture_data_process]: error while frame_size read\n");
            *exit_flag = 1;
            break;
        } 
        else if (bytes == 0) {
            usleep(1000);
            continue;
        }
        else { 
            //printf("header=%d bytes=%zd recieved=%zd remining=%zd\n", header_ok, bytes, recieved_len, remaining_len);
            recieved_len += bytes;
            if (!header_ok && recieved_len < iov[0].iov_len) {
                remaining_len -= bytes;
            }
            if (!header_ok && recieved_len == iov[0].iov_len) {
                data_size = *((size_t*)iov[0].iov_base);
                frame_id = *((int*)((char*)iov[0].iov_base + sizeof(size_t)));
                //fprintf(stdout, "[tcp_capture_data_process]: found_image_size = %zd found_frame_id = %d\n", data_size, frame_id);
                if (data_size != TEST_DATA_SIZE) {
                    fprintf(stdout, "[tcp_capture_data_process]: strange data size = %zd instead of %zd\n", data_size, TEST_DATA_SIZE);
                }
#if 0
                recieved_len -= iov[0].iov_len;
                remaining_len = iov[1].iov_len - recieved_len;
#else
                recieved_len = 0;
                remaining_len = data_size;
#endif
                header_ok = 1;;
            }
            if (header_ok && recieved_len < iov[1].iov_len) {
                remaining_len -= bytes;
            }
            if (header_ok && recieved_len == iov[1].iov_len) {
                if (imgSize != recieved_len) {
                        fprintf(stdout, "[tcp_capture_data_process]: result size = %zd instead of %zd\n", recieved_len, imgSize);
                }
//------------------------------------------------------------------------------------
                float radius = findRadius(img);
                //not smart algotithm of object detection
                //fprintf(stdout, "[tcp_capture_data_process]: frames_obj_cnt = %d frames_empty_cnt %d frames_non_cnt %d\n", frames_obj_cnt, frames_empty_cnt, frames_non_cnt);
                if (radius < 135 && new_object == 0) {
                    frames_empty_cnt++;
                    if (frames_obj_cnt >= 16 && frames_empty_cnt >= 8) {
                        new_object = 1;
                        fprintf(stdout, "[tcp_capture_data_process]: object break is detected after %d frames\n", frames_empty_cnt + frames_obj_cnt + frames_non_cnt);
                        //finalize current full object
                        sem_post(&collector->obj_ready);
                        //prepare new empty object
                        new_object = 0;
                        frames_obj_cnt = 0; 
                        frames_non_cnt = 0;
                        frames_empty_cnt = 0;
                    }
                }
                else if (radius >= 150){
                   frames_obj_cnt++;
                   frames_obj_cnt += frames_empty_cnt;
                   frames_empty_cnt = 0;
                   frames_obj_cnt += frames_non_cnt;
                   frames_non_cnt = 0;
                }
                else {
                    frames_non_cnt++;
                }
//-------------------------------------------------------------------------------------
                //sprintf(name, "../data/test_%.4d.bmp", id);
                //cv::imwrite(name, img);
                remaining_len = iov[0].iov_len;
                recieved_len = 0;
                header_ok = 0;
                fprintf(stdout, "Frame %d with frame_id %d is ready, radius = %f \n", id++, frame_id, radius);
            }
        }
    }

    fprintf(stdout, "[collect_data_thread]: stop\n");
    return NULL;
}

struct obj_collector *collector_init(char* addr, char *port) {
    struct obj_collector *collector = (struct obj_collector *)malloc(sizeof(struct obj_collector));
    if (!collector) {
        fprintf(stderr, "[collector_init]: can't allocte collector\n");
        fflush(stderr);
        return NULL;
    }

    struct addrinfo	hints;
    struct addrinfo	*result, *rp;
    int rc, fd;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;     //IPv4 && IPv6
    hints.ai_socktype = SOCK_STREAM; //TCP socket
    hints.ai_flags = 0;
    hints.ai_protocol = 0;           //Any protocol

    rc = getaddrinfo(addr, port, &hints, &result);
    if (rc != 0) {
	    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
	    return NULL;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
	    fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
	    if (fd == -1) continue;
	    if (connect(fd, rp->ai_addr, rp->ai_addrlen) != -1) break;
	    close(fd);
    }

   	if (rp == NULL) {
	    fprintf(stderr, "Could not connect\n");
	    return NULL;
    }

    freeaddrinfo(result);

    strcpy(collector->addr, addr);
    strcpy(collector->port, port);
    collector->fd = fd;
    collector->exit_flag = 0;
    
    if (sem_init(&(collector->obj_ready), 1, 0) == -1) {
	    fprintf(stderr, "[cvclient]: can't init object collector semaphore\n");
	    return NULL;
    }

    pthread_create(&collector->thread_id, NULL, collect_data_thread, collector);

    return collector;
}

void collector_destroy(struct obj_collector *collector){
    collector->exit_flag = 1;
    pthread_join(collector->thread_id, NULL);
    close(collector->fd);
    //TODO: when sem_wait???
    free(collector);
}

int main(int argc, char *argv[]){
    int obj_id = 0;
    //TODO: array of collectors
    struct obj_collector *collector = collector_init(argv[1], argv[2]);
    if (!collector) {
        fprintf(stdout, "[cvclient]: Can't init collector, may be camera isn't ready. Please, try again.\n");
        fflush(stdout);
        return 0;
    }
    while (!collector->exit_flag) {
        sem_wait(&collector->obj_ready);
        fprintf(stdout, "[cvclient]: New object %d ready\n", obj_id++);
        fflush(stdout);
    }
    if (collector) collector_destroy(collector);
    return 0;
}



