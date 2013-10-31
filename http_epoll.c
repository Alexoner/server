/*************************************************************************
    > File Name: http_epoll.c
    > Author: onerhao
# mail: onerhao@gmail.com
    > Created Time: Mon 28 Oct 2013 09:16:37 PM CST
 ************************************************************************/

#include "httpd.h"
#include "http_epoll.h"


/**There should be unified data structure registered with epoll
 * otherwise,it's impossible to identify the data.ptr field's object type
 **/


http_epoll_t http_epoll;

int http_epoll_init(http_epoll_t *this,int size)
{
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
	http_event_t *http_event;
    int i;
    int nfds;
	int fd=-1;

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
		else if(epoll_event->events &(EPOLLIN | EPOLLOUT) )
		{//ready for read
			printf("EPOLLIN | EPOLLOUT\n");
			http_event = (http_event_t*)epoll_event->data.ptr;
			//get the corresponding http_event associated with the epoll
			//event or fd
			http_event->event_handle(this,epoll_event);
			continue;
		}
    }
    return 0;
}

/*int http_epoll_event_handle(http_epoll_t *this,struct epoll_event *ee)
{
	http_event_t *e=ee->data.ptr;
	connection_t *c=e->data;
	if(!e || !c)
	{
		fprintf("http_epoll_event_handle: NULL pointer\n");
		return -1;
	}
	e->event_handle(this,e);
    return 0;
}*/

int http_accept_handle(http_epoll_t *this,struct epoll_event *eevent)
{
    connection_t *c=(connection_t*)malloc(sizeof(connection_t));
	http_event_t *old_event=(http_event_t*)eevent->data.ptr;
	http_event_t *new_event=(http_event_t*)malloc(sizeof(*new_event));
    struct epoll_event event;
    int flags;
	//int fd=old_event->listenfd;
    memset(c,0,sizeof(*c));
	memset(new_event,0,sizeof(*new_event));
	new_event->listenfd=-1;
	new_event->data=c;
	new_event->event_handle=http_connection_handle;

    c->fd=accept(old_event->listenfd,&c->addr,&c->addr_len);
    if(c->fd==-1)
    {
        perror("accpet()");
        return errno;
    }
    else
    {
        flags=O_RDWR |O_NONBLOCK;
        if(fcntl(c->fd,F_SETFL,flags)==-1)
		{
			perror("fcntl");
			return -1;
		}
		printf("New connection from %s: %d\tsocket: %d",
				inet_ntoa(((struct sockaddr_in*)&c->addr)->sin_addr),
				((struct sockaddr_in *)&c->addr)->sin_port,
				c->fd);
        event.data.ptr=new_event;
        event.events=EPOLLIN | EPOLLET| EPOLLERR |EPOLLHUP;
        //c->handle=http_connection_handle;
        http_epoll_add(this,c->fd,&event);
    }
    return 0;
}

int http_epoll_add_listen_socket(http_epoll_t *this,int fd)
{
	struct epoll_event event;
	http_event_t *e=(http_event_t*)malloc(sizeof(*e));
	int flags=0;
	memset(&event,0,sizeof(event));

	e->listenfd=fd;
	e->event_handle=http_accept_handle;
	flags=O_RDWR | O_NONBLOCK;
	if(fcntl(e->listenfd,F_SETFL,flags)==-1)
	{
		perror("epoll add listen socket");
		return -1;
	}

	printf("Added listen socket %d to epoll instance\n",fd);

	event.data.ptr=e;
	//event.events=EPOLLIN |EPOLLET | EPOLLERR | EPOLLHUP ;
	event.events=EPOLLIN | EPOLLERR | EPOLLHUP ;
	if(http_epoll_add(this,fd,&event)!=0)
	{
		return errno;
	}
	return 0;
}

int http_connection_handle(http_epoll_t *this,struct epoll_event *eevent)
{
	connection_t *c=((http_event_t*)eevent->data.ptr)->data;
	if(eevent->events & EPOLLIN)
	{
		if(accept_request(c->fd,&c->request.str))
		{
			parse_request(c->request);
			//strncpy(c->path,c->request.path,sizeof(c->path));
			c->sent=0;
			if(c->request.cgi)
			{//executable file for dynamic request
			}
			else if(!strcmp(c->request.method,"GET"))
			{//static files
				if(S_ISDIR(c->request.st.st_mode))
				{//request for directory

				}
				else
				{
					if((c->fd=open(c->request.path,O_RDONLY))==-1)
					{
						fprintf(stderr,"open %s error\n",c->request.path);
					}
				}
			}
		}
	}
	else
	{//EPOLLOUT
		http_send(c);
		if(c->request.st.st_size==c->sent)
		{//sending data finished
			c->fin=1;
			http_epoll_del(this,c->connfd);
			close(c->connfd);
			close(c->fd);
			free(c);
			free(eevent);
		}
	}
    return 0;
}

int http_request_handle(request_t request)
{
	return 0;
}


int http_send(connection_t *c)
{
	int n,ns=0,nr=0;
	char buf[MAXLINE];
	if(lseek(c->fd,c->sent,SEEK_SET)==-1)
	{
		return -1;
	}
	while(1)
	{
		nr=read(c->fd,buf,sizeof(buf));
		if(nr>0)
		{
			n=send(c->connfd,buf,nr,0);
			if(n>0)
			{
				ns+=n;
			}
		}
		else if(nr==0)
		{//end of file
			break;
		}
		else
		{//error
			if(errno==EAGAIN)
			{
				fprintf(stderr,"EAGAIN\n");
				break;
			}
			else
			{
				return -1;
			}
		}
	}
	c->sent+=ns;
	return ns;
}

