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

#define TEST_DATA_CNT  512
#define TEST_HEADER_SIZE sizeof(size_t)
#define TEST_RAW_WIDTH 2448
#define TEST_RAW_HEIGHT 2048
#define TEST_RAW_DEPTH 8
#define TEST_RAW_SIZE ( TEST_RAW_WIDTH * TEST_RAW_HEIGHT * TEST_RAW_DEPTH / 8 )
#define TEST_DATA_SIZE ( TEST_HEADER_SIZE + TEST_RAW_SIZE )


#if 0
void* collect_data_thread(void *arg) {
    pthread_detach(pthread_self());
    int *fd = (int*)arg;
    int ln = -1;
    char *buf = NULL;
    int buf_size = 0; 
    while (1) {
        buf = (char*)tcp_get_next_image();
        buf_size = (int)(
        printf("[%d]: size = %d, buf = %s", curr, buf_size, buf);
        ln = read(*fd, buf, sizeof(buf));
        if ((ln < 0) || (ln != buf_size)) {
            break;
        }
    }
    close(*fd);
    return NULL;
}
#endif

void nextFrameReady(struct iovec io) {
#if 0
    if (imgSize != io.iov_len - TEST_HEADER_SIZE) {
        fprintf(stdout, "[nextFrameReady]: problems with result size\n");
    }
    sprintf(name, "../data/test_%.4d.bmp", id++);
    cv::imwrite(name, img);
#endif
}

int tcp_capture_data_process(char* addr, char *port) {
    int			fd;
    struct addrinfo	hints;
    struct addrinfo	*result, *rp;
    int			rc;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;     //IPv4 && IPv6
    hints.ai_socktype = SOCK_STREAM; //TCP socket
    hints.ai_flags = 0;
    hints.ai_protocol = 0;           //Any protocol

    rc = getaddrinfo(addr, port, &hints, &result);
    if (rc != 0) {
			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
			exit(EXIT_FAILURE);
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
			fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
			if (fd == -1) continue;
			if (connect(fd, rp->ai_addr, rp->ai_addrlen) != -1) break;
			close(fd);
    }

   	if (rp == NULL) {
			fprintf(stderr, "Could not connect\n");
			exit(EXIT_FAILURE);
    }

    freeaddrinfo(result);
     
    cv::Mat img;
    img = cv::Mat::zeros(TEST_RAW_HEIGHT, TEST_RAW_WIDTH, CV_8UC1);
    int imgSize = img.total() * img.elemSize();
    
    size_t  data_size = 0;
    struct iovec iov[2];
    
    iov[0].iov_base = (size_t*)&data_size;
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
    while (1) {
        if (remaining_len == 0) {
            fprintf(stdout, "wtf???\n");
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
            fprintf(stdout, "2\n");
            bytes = read(fd, (char*)iov[0].iov_base + recieved_len, remaining_len);
        }
        else {
            fprintf(stdout, "3: recv = %zd, rem = %zd\n", recieved_len, remaining_len);
            bytes = read(fd, (char*)iov[1].iov_base + recieved_len, remaining_len);
        }
        if (bytes < 0) {
            fprintf(stdout, "[tcp_capture_data_process]: error while frame_size read\n");
            break;
        } 
        else if (bytes == 0) {
            continue;
        }
        else { 
            printf("header=%d bytes=%zd recieved=%zd remining=%zd\n", header_ok, bytes, recieved_len, remaining_len);
            recieved_len += bytes;
            if (!header_ok && recieved_len < iov[0].iov_len) {
                remaining_len -= bytes;
            }
            if (!header_ok && recieved_len == iov[0].iov_len) {
                data_size = *((size_t*)iov[0].iov_base);
                fprintf(stdout, "[tcp_capture_data_process]: found_image_size = %zd\n", data_size);
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
                //nextFrameReady(img);
                if (imgSize != recieved_len) {
                        fprintf(stdout, "[tcp_capture_data_process]: result size = %zd instead of %zd\n", recieved_len, imgSize);
                }
                sprintf(name, "../data/test_%.4d.bmp", id++);
                cv::imwrite(name, img);
                remaining_len = iov[0].iov_len;
                recieved_len = 0;
                header_ok = 0;
                fprintf(stdout, "Frame %d is ready\n", id - 1);
            }
        }
    }

    close(fd);
    return EXIT_SUCCESS;
}


int main(int argc, char *argv[]){
    tcp_capture_data_process(argv[1], argv[2]);
    return 0;
}



