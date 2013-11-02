/*************************************************************************
    > File Name: http_epoll.h
    > Author: onerhao
# mail: onerhao@gmail.com
    > Created Time: Wed 30 Oct 2013 09:26:58 PM CST
 ************************************************************************/

#ifndef HTTP_EPOLL_H
#define HTTP_EPOLL_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/epoll.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "httpd.h"
#include "http_epoll.h"

typedef struct connection_s
{
    void *data;
    request_t request;
    int connfd; //connection socket file descriptor
    char path[MAX_PATH_L];
<<<<<<< HEAD
    int fd;    //request file descriptor
    off_t sent;
    int fin:1; //finished response
    struct sockaddr addr;
    socklen_t addr_len;
=======
    char headers[MAX_HEADERS_L];
    int fd;    //request file descriptor
    off_t sent;
    int read:1;//read or write
    int sendheaders:1;//send headers or file
    int senddata:1;   //error request and error page not found
    int fin:1; //finished response
    struct sockaddr_in addr;
    socklen_t addr_len;
    char ip[16];
    int port;
>>>>>>> develop
    int (*handle)(struct connection_s *c);
} connection_t;

typedef struct http_epoll_s
{
    //encapsulate basic data for epoll operation
    int epfd;
    int size;
    int used;
    struct epoll_event *events;
} http_epoll_t;

typedef struct http_event_s
{
    void *data;//points to the connction_t or a
    int listenfd;//listening file descriptor,-1 when it's not
    int (*event_handle)(http_epoll_t *this,struct epoll_event *eevent);
    //to EPOLL signal event are expected by this function
} http_event_t;


int http_epoll_init(http_epoll_t *this,int size);
int http_epoll_close(http_epoll_t *this);
int http_epoll_add(http_epoll_t *this,int fd,struct epoll_event *data);
<<<<<<< HEAD
int http_epoll_del(http_epoll_t *this,int fd);
int http_epoll_wait(http_epoll_t *this,int msec);
int http_epoll_event_handle(http_epoll_t *this,http_event_t *e);
int http_accept_handle(http_epoll_t *this,struct epoll_event *eevent);
int http_epoll_add_listen_socket(http_epoll_t *this,int fd);
int http_connection_handle(http_epoll_t *c,struct epoll_event *eevent);
int http_send(connection_t *c);

=======
int http_epoll_del(http_epoll_t *this,int fd,struct epoll_event *eevent);
int http_epoll_wait(http_epoll_t *this,int msec);
int http_epoll_event_handle(http_epoll_t *this,http_event_t *e);
int http_connection_error_page(connection_t *c,int errcode);
int http_connection_error_headers(connection_t *c,
		int code,const char *message);
int http_accept_handle(http_epoll_t *this,struct epoll_event *eevent);
int http_epoll_add_listen_socket(http_epoll_t *this,int fd);
int http_connection_handle(http_epoll_t *c,struct epoll_event *eevent);
int http_connection_headers(connection_t *c);
int http_send(connection_t *c);
int http_recv(connection_t *c);

//utils
int setnonblock(int fd);
>>>>>>> develop
#endif
