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

typedef struct client
{
	int fd;
	struct sockaddr addr;
	int (*handle)(int fd);
}client;

typedef struct http_epoll
{
	int epfd;
	int size;
	int used;
	struct epoll_event *events;
}http_epoll;

int http_epoll_init(struct http_epoll *this,int size)
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

int http_epoll_close(struct http_epoll *this)
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

int http_epoll_add(struct http_epoll *this,int fd)
{
	struct epoll_event event;

	event.data.fd=fd;
	event.events=EPOLLIN | EPOLLOUT |EPOLLERR |EPOLLHUP;

	if(epoll_ctl(this->epfd,EPOLL_CTL_ADD,fd,&event)==-1)
	{
		perror("epoll_ctl()");
		return errno;
	}

	this->used++;
	return 0;
}

int http_epoll_wait(struct http_epoll *this,int msec)
{
	struct epoll_event *epoll_event=NULL;
	int i;
	int nfds;

	nfds=epoll_wait(this->epfd,this->events,this->size,msec);

	if(nfds==-1)
		return errno;

	for(i=0;i<nfds;i++)
	{
		epoll_event=this->events+i;

		//TO handle these file descriptors

	}

	return 0;
}

