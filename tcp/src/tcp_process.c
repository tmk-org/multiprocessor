#include <getopt.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/select.h>
#if 0
    #include <netinet/in.h>
    #include <arpa/inet.h>
#else
  #include <netdb.h>
#endif

#include <unistd.h>

#include "tcp/tcp_process.h"

#define MAX_CONNECTION	10

#if 0
struct connection{
    LIST		entry;
    FILE		*f;
    int			broken;
    char		buffer[MAX_BUFFER_SIZE];
    size_t		buffer_used;

    int			player;
};
#endif

char *defaultExecuteCommand(const char *buf, int *exit_flag) {
    fprintf(stdout, "[defaultExecuteCommand]: TODO execute_command_function\n");
    fflush(stdout);
    if (strcasecmp(buf, "STOP") == 0 || strcasecmp(buf, "S") == 0) {
        fprintf(stdout, "[defaultExecuteCommand]: STOP command recieved\n");
        fflush(stdout);
        *exit_flag = 1;
    }
    return strdup("OK\n");
}
/*
char *getRequest() {
    static char s[80];
    fprintf(stdout, "Message: ");
    fflush(stdout);
	memset(s, 0, sizeof(s));
	scanf("%78[^\n]%*c", s);
	strcat(s, "\n");
    return s;
}
*/
int checkReply(char *buf) {
    if (buf != NULL) {
        return 0;
    }
    return -1;
}

//int tcp_server_process(char *port){
int tcp_server_process(char *port, char *(*exec_cmd) (const char *, int *)){
    int			exit_flag = 0, fd, client_cnt = 0;
    int			cfd[MAX_CONNECTION];

    struct addrinfo	hints;
    struct addrinfo	*result, *rp;
    int			rc, reuse_addr;
    
    if (exec_cmd == NULL) exec_cmd = defaultExecuteCommand; 

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
	    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
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
	    fprintf(stderr, "Could not bind\n");
	    fflush(stderr);
	    exit(EXIT_FAILURE);
    }

    freeaddrinfo(result);

    if (listen(fd, 10) != 0){
	    fprintf(stderr, "Could not listen, error: %s\n", strerror(errno));
	    fflush(stderr);
	    exit(EXIT_FAILURE);
    }

    while(!exit_flag){
	    fd_set	rfds;
	    int	i, retval, max;

	    FD_ZERO(&rfds);

	    max = fd;
	    FD_SET(fd, &rfds);
	    for(i = 0; i < client_cnt; i++){
		    if (cfd[i] > max) max = cfd[i];
		    FD_SET(cfd[i], &rfds);
	    }

	    retval = select(max + 1, &rfds, NULL, NULL, NULL);
	    if (retval == -1){
	        fprintf(stderr, "Could not select, error: %s\n", strerror(errno));
	        fflush(stderr);
	        exit(EXIT_FAILURE);
	    }

	    if (FD_ISSET(fd, &rfds)){
	        //new connection
	        struct sockaddr_storage	peer_addr;
	        socklen_t			peer_addr_len;
	        char			host[NI_MAXHOST], service[NI_MAXSERV];
	        int				fd1;

	        peer_addr_len = sizeof(peer_addr);
	        fd1 = accept(fd, (struct sockaddr *) &peer_addr, &peer_addr_len);
	        if (fd1 != -1){
		        retval = getnameinfo((struct sockaddr *) &peer_addr,
		                        peer_addr_len, host, NI_MAXHOST,
		                        service, NI_MAXSERV, NI_NUMERICSERV);
		        if (retval == 0) {
		            fprintf(stdout, "Received connection from %s:%s\n", host, service);
		            fflush(stdout);
		        }
		        else {
		            fprintf(stderr, "getnameinfo: %s\n", gai_strerror(retval));
                    fflush(stderr);
		        }
		        if (client_cnt < MAX_CONNECTION) {
		             cfd[client_cnt++] = fd1; 
		        }
		        else{
		            //close connection
		            fprintf(stderr, "Max connections reached, drop new connection\n");
                    fflush(stderr);
		            close(fd1);
		        }
	        }
	    }
#warning "List of clients instead of array"	
	    for(i = 0; i < client_cnt; i++){
	        ssize_t			bytes;
#warning "Buffer should be personal for each client, it should be concatinated"	        
	        char			buf[1000];

	        if (!FD_ISSET(cfd[i], &rfds)) continue;
	        //data by connection cfd[i]
	        bytes = read(cfd[i], buf, sizeof(buf));
	        if (bytes <= 0){
				    if (bytes < 0){
		       		    fprintf(stdout, "read error: %s\n", strerror(errno));
		       		    fflush(stdout);
				    }
				    fprintf(stdout, "close connection %d\n", i);
                    fflush(stdout);
				    close(cfd[i]);
				    memmove(&cfd[i], &cfd[i + 1], (client_cnt - i - 1) * sizeof(int));
				    client_cnt--;
				    i--;
				    continue;
	        }
	        
	        //working with data 
	        buf[bytes] = '\0';
	        fprintf(stdout, "[tcp_process_server]: {connection %d} %s\n", i, buf);
	        fflush(stdout);
	        //reply
	        char *reply = exec_cmd(buf, &exit_flag);
	        write(cfd[i], reply, strlen(reply));
	        free(reply);
	    }
    }
    close(fd);
    return 0;
}

#if 0
int tcp_server_with_command_queue_process(char *port) {
  int listen_sock = 0;
  int acc_sock = 0;
  struct sockaddr_in addr;
  char recv_buf[COMMAND_BUF_LEN];
  char command_buf[COMMAND_BUF_LEN];
  char *comm_sep;
  int rc, recv_pos, comm_len, remainder_len;
  command_t *new_comm;
  quit_cond_t *quit_cond;
  fd_set read_fds;
  struct timeval tv;
  int err;
  int isSocketReuseAddr = 1;

  listen_sock = socket(PF_INET, SOCK_STREAM, 0);
  if (-1 == listen_sock)
  {
    LOGCSER("socket failed\n");
    return -1;
  }
  setcloexec(listen_sock);

  err = setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, (char*)&isSocketReuseAddr, sizeof(isSocketReuseAddr));
  if (err) {
      LOGCSER("socket options failed\n");
      close(listen_sock);
      return -1;
  }
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(listen_port);

  if (0 != bind(listen_sock, (struct sockaddr *)&addr, sizeof(addr)))
  {
    LOGCSER("bind failed\n");
    close(listen_sock);
    return -1;
  }

  if (0 != listen(listen_sock, 1))
  {
    LOGCSER("listen failed\n");
    close(listen_sock);
    return -1;
  }

  while (1) {
    acc_sock = accept(listen_sock, NULL, NULL);
    if (acc_sock == -1) {
      /* Non-fatal errors */
      if ((errno == ECONNABORTED) || (errno == EINTR) ||
          (errno == EAGAIN) || (errno == EWOULDBLOCK))
        continue;
      LOGCSER("accept failed: %s\n", strerror(errno));
      close(listen_sock);
      return -1;
    }
    setcloexec(acc_sock);
    break;
  }

  quit_cond = malloc(sizeof(quit_cond_t));
  quit_cond_init(quit_cond);
  if (0 != init_command_server(quit_cond))
  {
    LOGCSER("init_command_server failed\n");
    close(listen_sock);
    close(acc_sock);
    quit_cond_destroy(quit_cond);
    free(quit_cond);
    quit_cond = NULL;
    return -1;
  }

  memset(command_buf, 0, COMMAND_BUF_LEN);
  recv_pos = 0;

  // Main loop
  while (1)
  {
    // Check quit cond
    if (quit_cond_check(quit_cond))
    {
      LOGCS("quit_cond signalled - quitting\n");
      break;
    }

    FD_ZERO(&read_fds);
    FD_SET(acc_sock, &read_fds);
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    rc = select(acc_sock + 1, &read_fds, NULL, NULL, &tv);
    if (0 == rc) // Timeout
    {
      continue;
    }
    else if (rc < 0)
    {
      LOGCSER("Error during select '%d'\n", errno);
    }

    if (!FD_ISSET(acc_sock, &read_fds))
    {
      LOGCSER("acc_sock is not set in read_fds\n");
      continue;
    }

#ifdef PRINT_PACKET_INFO
    LOGCS("recv_pos = %d\n", recv_pos);
#endif
    rc = recv(acc_sock, recv_buf + recv_pos, COMMAND_BUF_LEN - recv_pos - 1, MSG_NOSIGNAL|MSG_DONTWAIT);
    if (rc < 0)
    {
      LOGCSER("Error during command recv: %d\n", errno);
      set_queue_ready();
      break;
    }
    else
    {
#ifdef PRINT_PACKET_INFO
      LOGCS("Received %d bytes\n", rc);
#endif
    }

    if (0 == rc)
    {
      LOGCS("Connection was closed by client\n");
      // command_ready should be put on here!!!
      set_queue_ready();
      break;
    }

    recv_pos += rc;

    // Parse accumulated command line
    while (1)
    {
      comm_sep = (char *)strchr(recv_buf, '\n');
      if (NULL == comm_sep)
      {
        LOGCSER("Cannot find end of command, recv_buf = '%s'\n", recv_buf);
        break;
      }

      comm_len = comm_sep - recv_buf;
      remainder_len = recv_pos - comm_len - 1;

      strncpy(command_buf, recv_buf, comm_len);
      command_buf[comm_len] = '\0';

      memmove(recv_buf, recv_buf + comm_len + 1, remainder_len);
      memset(recv_buf + remainder_len, 0, COMMAND_BUF_LEN - remainder_len);
      recv_pos = remainder_len;

      // Create command and add into queue
      new_comm = (command_t *)malloc(sizeof(command_t));

      memset(new_comm, 0, sizeof(command_t));
      strncpy(new_comm->full_command, command_buf, comm_len);
      new_comm->client_sock = acc_sock;
#ifdef PRINT_PACKET_INFO
      LOGCS("Queuing new command: '%s'\n", new_comm->full_command);
#endif
      add_command(new_comm);
      if (*recv_buf == '\0')
        break;
    }
  }
  close(acc_sock);
  close(listen_sock);
  stop_command_server();
  quit_cond_destroy(quit_cond);
  free(quit_cond);

  return 0;
}

#endif

int tcp_client_process(char* addr, char *port){
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

    while(1){
		ssize_t	rc = 0;
        char cmd[MESSAGE_LEN];
		char reply[MESSAGE_LEN];

		strcpy(cmd, getRequest());

		rc = write(fd, cmd, strlen(cmd));
		if (rc != strlen(cmd)) {
	        if (rc < 0) {
	            printf("write error: %s\n", strerror(errno));
	        }
	        break;
		}
		char s[80];
	    rc = read(fd, s, sizeof(s) - 1);
	    if (rc <= 0) {
	       	if (rc < 0) {
	       	    printf("read error: %s\n", strerror(errno));
	       	}
	       	break;
		}
		s[rc] = '\0';

		if (checkReply(s) < 0) {
		    strcpy(reply, "no answer");
		}
		else {
		    strcpy(reply, s);
		}

		parseReply(reply); //printf("server reply: %s\n", reply);
    }

    close(fd);
    return EXIT_SUCCESS;
}

int tcp_client_with_select_process(char* addr, char *port) {

  int fd, rc = 0;

  char buf[MESSAGE_LEN];
  int isQuiting = 0;
  long mode = 0;
  fd_set read_fds;
  struct timeval tv;

#if 0
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;     //IPv4
    addr.sin_addr.s_addr = inet_addr(addr);
    addr.sin_port = htons(program_opts.port);

    if (0 != connect(fd, (struct sockaddr *)&addr, sizeof(addr))) {
        printf("connect failed\n");
        close(sock);
        return EXIT_FAILURE;
    }
#else
    struct addrinfo	hints;
    struct addrinfo	*result, *rp;
    
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;     // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP socket
    hints.ai_flags = 0;
    hints.ai_protocol = 0;           // any protocol

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
#endif

  while (!isQuiting) {
    printf("\nEnter command: ");
    if (!fgets(buf, MESSAGE_LEN, stdin)) {
      continue;
    }

    if (buf[0] == '\n')
      continue;

    if (buf[strlen(buf) - 1] != '\n') {
      printf("\nERROR: invalid command.\n");

      fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
      while (getchar() != -1);
      mode = fcntl(STDIN_FILENO, F_GETFL);
      fcntl(STDIN_FILENO, F_SETFL, mode & ~O_NONBLOCK);

      continue;
    }

    if (buf[0] == '_')
        break;

    if (!strcasecmp(buf, "QUIT\n") || !strcasecmp(buf, "Q\n"))
      isQuiting = 1;

    rc = send(fd, buf, strlen(buf), 0);
    if (rc < 0)
      printf("Sending failed, errno = %d\n", errno);
    else
      printf("Command successfully sent\n");

    rc = 0;
    while (!rc) {
      FD_ZERO(&read_fds);
      FD_SET(fd, &read_fds);
      tv.tv_sec = 1;
      tv.tv_usec = 0;
      rc = select(fd + 1, &read_fds, NULL, NULL, &tv);
      if (rc < 0) {
        printf("[tcp_client_process]: select failed while waiting for reply: %s\n", strerror(errno));
        continue;
      } 
      else {
        printf("Waiting for reply...\n");
        continue;
      }
      if (!FD_ISSET(fd, &read_fds))
        continue;
    }

#warning "Buffer shoud be concatinated"    
    rc = recv(fd, buf, MESSAGE_LEN - 1, MSG_NOSIGNAL|MSG_DONTWAIT);
    if (rc < 0) {
      printf("Recv failed: %s\n", strerror(errno));
      continue;
    } 
    else if (rc > 0) {
      printf("Reply received, rc = %d\n", rc);
      buf[rc] = '\0';
    } 
    else {
      printf("Connection closed by server\n");
      break;
    }
    printf("Reply: %s\n", buf);
  }

  close(fd);

  return EXIT_SUCCESS;
}






