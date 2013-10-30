/*************************************************************************
    > File Name: http_epoll.c
    > Author: onerhao
# mail: onerhao@gmail.com
    > Created Time: Mon 28 Oct 2013 09:16:37 PM CST
 ************************************************************************/

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

typedef struct http_event_s
{
    void *data;//points to the connction_t or a
    int listenfd;//listening file descriptor,-1 when it's not
    int (*event_handle)(struct http_event_s *e);
} http_event_t;

typedef struct connection_s
{
    void *data;
    int fd; //connection socket file descriptor
    char path[MAX_PATH_L];
    off_t sent;
    struct sockaddr addr;
    socklen_t addr_len;
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

int http_epoll_init(http_epoll_t *this,int size)
{
    int i;

    if(size<=0)
    {
        return -1;
    }

    this->size=size;
    this->used=0;
    this->events=NULL;

    this->epfd=epoll_create(this->size);
    if(this->epfd==-1)
    {
        return errno;
    }

    this->events=malloc(sizeof(struct epoll_event)*this->size);
    if(this->events)
    {
        return ENOMEM;
    }

    return 0;
}

int http_epoll_close(http_epoll_t *this)
{
    if(this->events)
    {
        free(this->events);
    }
    close(this->epfd);
    this->size=0;
    this->used=0;
    this->epfd=-1;
    return 0;
}

int http_epoll_add(http_epoll_t *this,int fd,struct epoll_event *data)
{
    //register fd on the epoll instance and associate data with it
    struct epoll_event event;

    if(!data)
    {
        fprintf(stderr,"NULL epoll_event passed\n");
        return -1;
    }
    else
    {
        memcpy(&event,data,sizeof(event));
    }

    if(epoll_ctl(this->epfd,EPOLL_CTL_ADD,fd,&event)==-1)
    {
        perror("epoll_ctl()");
        return errno;
    }

    this->used++;
    return 0;
}

int http_epoll_del(http_epoll_t *this,int fd)
{
    if(epoll_ctl(this->epfd,EPOLL_CTL_DEL,fd,NULL)==-1)
    {
        perror("epoll_ctl()");
        return errno;
    }
    this->used--;
    return 0;
}

int http_epoll_wait(http_epoll_t *this,int msec)
{
    struct epoll_event *epoll_event=NULL;
    int i;
    int nfds;

    nfds=epoll_wait(this->epfd,this->events,this->used,msec);

    if(nfds==-1)
    {
        perror("epoll_wait");
        return errno;
    }

    for(i=0; i<nfds; i++)
    {
        epoll_event=this->events+i;
		if(epoll_event->events & EPOLLERR)
		{
			fprintf(stderr,"EPOLLERR occured on fd\n");
			continue;
		}
		else if(epoll_event->events & EPOLLHUP)
		{
			fprintf(stderr,"EPOLLHUP occured on fd\n");
			continue;
		}
		else if(epoll_event->events &EPOLLIN )
		{
			http_connection_handle(
			continue;
		}
        //TO handle these file descriptors
        if(((http_event_t*)epoll_event->data.ptr)->event_handle(epoll_event)!=0)
        {
            fprintf(stderr,"handling error for %dth event\n",i);
        }
    }
    return 0;
}

int http_epoll_event_handle(http_event_t *e)
{
    if (event->events & EPOLLIN)
    {
    }
    else
    {
        switch (event->events)
        {
        case EPOLLOUT:
        {
            //apex_log_info("fd %d Event:EPOLLOUT %d\n", fd, event->events);
            if(data)
            {
                //send( fd,data,strlen(data),0);
                //apex_log_info("Event:EPOLLOUT %s\n", data);
                apex_http_response((char*)data, &apex_epoll, fd);
            }
            apex_epoll_set( &apex_epoll, fd, EPOLLIN, NULL);
        }
        break;

        case EPOLLERR:
            apex_log_info("fd %d Event:EPOLLERR %d\n", fd, event->events);
            break;
        case EPOLLHUP:
            apex_log_info("fd %d Event:EPOLLHUP %d\n", fd, event->events);
            apex_epoll_del( &apex_epoll, fd);
            break;
        default:
            apex_log_info("fd %d Event: %d\n", fd, event->events);
            break;
        }
    }

    return 0;
}

int http_epoll_accept(http_epoll_t *this,http_event_t *e)
{
    connection_t c;
    struct epoll_event event;
    int flags;
    memset(&c,0,sizeof(c));

    c.fd=accept(e->listenfd,&c.addr,&c.addr_len);
    if(c.fd==-1)
    {
        perror("accpet()");
        return errno;
    }
    else
    {
        flags=O_RDWR |O_NONBLOCK;
        fcntl(c.fd,F_SETFL,flags);
        event.data.ptr=&c;
        event.events=EPOLLIN |EPOLLERR |EPOLLHUP;
        c.handle=http_connection_handle;
        http_epoll_add(this,c.fd,&event);
    }
    return 0;
}

int http_connection_handle(connection_t *c)
{
    return 0;
}

