/*****************************************************************************
#                                                                            *
# Copyright 2019 AT&T Intellectual Property                                  *
# Copyright 2019 Nokia                                                       *
#                                                                            *
# Licensed under the Apache License, Version 2.0 (the "License");            *
# you may not use this file except in compliance with the License.           *
# You may obtain a copy of the License at                                    *
#                                                                            *
#      http://www.apache.org/licenses/LICENSE-2.0                            *
#                                                                            *
# Unless required by applicable law or agreed to in writing, software        *
# distributed under the License is distributed on an "AS IS" BASIS,          *
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
# See the License for the specific language governing permissions and        *
# limitations under the License.                                             *
#                                                                            *
******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <unistd.h>		//for close()
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>	//for inet_ntop()
#include <assert.h>

#include "e2sim_sctp.hpp"
// #include "e2sim_defs.h"
#include "logger.h"

#include <sys/types.h>
#include <netdb.h>
#include <signal.h>
#include <time.h>

int sctp_start_server(const char *server_ip_str, const int server_port)
{
  if(server_port < 1 || server_port > 65535) {
      logger_error("Invalid port number (%d). Valid values are between 1 and 65535.", server_port);
      exit(1);
  }

  int server_fd, af;
  struct sockaddr* server_addr;
  size_t addr_len;

  struct sockaddr_in  server4_addr;
  memset(&server4_addr, 0, sizeof(struct sockaddr_in));

  struct sockaddr_in6 server6_addr;
  memset(&server6_addr, 0, sizeof(struct sockaddr_in6));

  if(inet_pton(AF_INET, server_ip_str, &server4_addr.sin_addr) == 1)
  {
    server4_addr.sin_family = AF_INET;
    server4_addr.sin_port   = htons(server_port);

    server_addr = (struct sockaddr*)&server4_addr;
    af          = AF_INET;
    addr_len    = sizeof(server4_addr);
  }
  else if(inet_pton(AF_INET6, server_ip_str, &server6_addr.sin6_addr) == 1)
  {
    server6_addr.sin6_family = AF_INET6;
    server6_addr.sin6_port   = htons(server_port);

    server_addr = (struct sockaddr*)&server6_addr;
    af          = AF_INET6;
    addr_len    = sizeof(server6_addr);
  }
  else {
    perror("inet_pton()");
    exit(1);
  }

  if((server_fd = socket(af, SOCK_STREAM, IPPROTO_SCTP)) == -1) {
    perror("socket");
    exit(1);
  }

  //set send_buffer
  // int sendbuff = 10000;
  // socklen_t optlen = sizeof(sendbuff);
  // if(getsockopt(server_fd, SOL_SOCKET, SO_SNDBUF, &sendbuff, &optlen) == -1) {
  //   perror("getsockopt send");
  //   exit(1);
  // }
  // else
  //   LOG_D("[SCTP] send buffer size = %d\n", sendbuff);


  if(bind(server_fd, server_addr, addr_len) == -1) {
    perror("bind");
    exit(1);
  }

  if(listen(server_fd, SERVER_LISTEN_QUEUE_SIZE) != 0) {
    perror("listen");
    exit(1);
  }

  assert(server_fd != 0);

  logger_info("[SCTP] Server started on %s:%d", server_ip_str, server_port);

  return server_fd;
}

/*
  Starts a client SCTP connection to a given server and port.
  Accepts IPv4, IPv6, or hostname as input values.
*/
int sctp_start_client(const char *server_addr_str, const int server_port) {
  int client_fd;
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  int ret;

  /* Obtain address(es) matching host/port */
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = 0;
  hints.ai_protocol = IPPROTO_SCTP;

  char port_buf[6];
  snprintf(port_buf, 6, "%d", server_port);

  ret = getaddrinfo(server_addr_str, port_buf, &hints, &result);
  if (ret != 0) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
      exit(EXIT_FAILURE);
  }

  logger_info("[SCTP] Connecting to server at %s:%d ...", server_addr_str, server_port);

  /* getaddrinfo() returns a list of address structures (IPv4 and IPv6).
    We have to try each address until we successfully connect. */
  for (rp = result; rp != NULL; rp = rp->ai_next) {
      client_fd = socket(rp->ai_family, rp->ai_socktype,
                      rp->ai_protocol);
      if (client_fd == -1)
          continue;

      if (connect(client_fd, rp->ai_addr, rp->ai_addrlen) != -1)
          break;                  /* Success */

      close(client_fd);
  }

  if (rp == NULL) {               /* No address succeeded */
      logger_fatal("Unable to connect to %s", server_addr_str);
      exit(EXIT_FAILURE);
  }

  freeaddrinfo(result);           /* No longer needed */

  logger_info("[SCTP] Connection established");
  logger_debug("[SCTP] client_fd value is %d", client_fd);

  return client_fd;
}

int sctp_accept_connection(const char *server_ip_str, const int server_fd)
{
  logger_info("[SCTP] Waiting for new connection...");

  struct sockaddr client_addr;
  socklen_t       client_addr_size;
  int             client_fd;

  //Blocking call
  client_fd = accept(server_fd, &client_addr, &client_addr_size);
  logger_debug("client fd is %d", client_fd);
  if(client_fd == -1){
    perror("accept()");
    close(client_fd);
    exit(1);
  }

  //Retrieve client IP_ADDR
  char client_ip6_addr[INET6_ADDRSTRLEN], client_ip4_addr[INET_ADDRSTRLEN];
  if(strchr(server_ip_str, ':') != NULL) //IPv6
  {
    struct sockaddr_in6* client_ipv6 = (struct sockaddr_in6*)&client_addr;
    inet_ntop(AF_INET6, &(client_ipv6->sin6_addr), client_ip6_addr, INET6_ADDRSTRLEN);
    logger_info("[SCTP] New client connected from %s", client_ip6_addr);
  }
  else {
    struct sockaddr_in* client_ipv4 = (struct sockaddr_in*)&client_addr;
    inet_ntop(AF_INET, &(client_ipv4->sin_addr), client_ip4_addr, INET_ADDRSTRLEN);
    logger_info("[SCTP] New client connected from %s", client_ip4_addr);
  }

  return client_fd;
}

int sctp_send_data(int &socket_fd, sctp_buffer_t &data, struct timespec *ts)
{
  logger_trace("in func %s", __func__);
  logger_debug("data.len is %d", data.len);
  if(ts != NULL) {
    clock_gettime(CLOCK_REALTIME, ts);
  }
  int sent_len = send(socket_fd, (void*)(&(data.buffer[0])), data.len, 0);

  logger_trace("after getting sent_len");

  if(sent_len == -1) {
    perror("[SCTP] sctp_send_data");
    exit(1);
  }

  return sent_len;
}

int sctp_send_data_X2AP(int &socket_fd, sctp_buffer_t &data)
{
  /*
  int sent_len = sctp_sendmsg(socket_fd, (void*)(&(data.buffer[0])), data.len,
                  NULL, 0, (uint32_t) X2AP_PPID, 0, 0, 0, 0);

  if(sent_len == -1) {
    perror("[SCTP] sctp_send_data");
    exit(1);
  }
  */
  return 1;
}

/*
Receive data from SCTP socket
Outcome of recv()
-1: exit the program
0: close the connection
+: new data
*/
int sctp_receive_data(int &socket_fd, sctp_buffer_t &data, struct timespec *ts)
{
  //clear out the data before receiving
  logger_trace("in func %s", __func__);
  memset(data.buffer, 0, sizeof(data.buffer));
  data.len = 0;

  //receive data from the socket
  int recv_len = recv(socket_fd, &(data.buffer), sizeof(data.buffer), 0);
  if(ts != NULL) {
    if(recv_len > 0) {
      clock_gettime(CLOCK_REALTIME, ts);
    } else {
      memset(ts, 0, sizeof(struct timespec));
    }
  }
  logger_debug("[SCTP] received %d bytes", recv_len);

  if(recv_len == -1)
  {
    perror("[SCTP] recv");
    exit(1);
  }
  else if (recv_len == 0)
  {
    logger_info("[SCTP] Connection closed by remote peer");
    if(close(socket_fd) == -1)
    {
      perror("[SCTP] close");
    }
    return -1;
  }

  data.len = recv_len;

  return recv_len;
}
