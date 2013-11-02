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

<<<<<<< HEAD
int http_epoll_add(http_epoll_t *this,int fd,struct epoll_event *data)
{
    //register fd on the epoll instance and associate data with it
    struct epoll_event event;

    if(!data)
=======
int http_epoll_add(http_epoll_t *this,int fd,struct epoll_event *eevent)
{
    //register fd on the epoll instance and associate data with it
    if(!eevent)
>>>>>>> develop
    {
        fprintf(stderr,"NULL epoll_event passed\n");
        return -1;
    }
<<<<<<< HEAD
    else
    {
        memcpy(&event,data,sizeof(event));
    }

    if(epoll_ctl(this->epfd,EPOLL_CTL_ADD,fd,&event)==-1)
    {
        perror("epoll_ctl()");
=======

    if(epoll_ctl(this->epfd,EPOLL_CTL_ADD,fd,eevent)==-1)
    {
        perror("http_epoll_add->epoll_ctl()");
>>>>>>> develop
        return errno;
    }

    this->used++;
<<<<<<< HEAD
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
=======
	//fprintf(stderr,"this->used increased to %d\n",this->used);
    return 0;
}

int http_epoll_del(http_epoll_t *this,int fd,struct epoll_event *eevent)
{
	http_event_t *e=(http_event_t*)eevent->data.ptr;
	connection_t *c=e->data;
    if(epoll_ctl(this->epfd,EPOLL_CTL_DEL,fd,NULL)==-1)
    {
        perror("http_epoll_del->epoll_ctl()");
        return errno;
    }
    this->used--;
	close(c->connfd);
	fprintf(stderr,"closed socket %d on %s\n",c->connfd,c->ip);
	if(c->fd>0)
	{
		close(c->fd);
	}
	if(c)
	{
		if(c->request.str)
			free(c->request.str);
		if(c->request.arg)
			free(c->request.arg);
		free(c);//free the memory of connection_t in heap
	}
	if(e)
		free(e);//free the memory of http_event_t in heap
>>>>>>> develop
    return 0;
}

int http_epoll_wait(http_epoll_t *this,int msec)
{
    struct epoll_event *epoll_event=NULL;
	http_event_t *http_event;
    int i;
    int nfds;
	//int fd=-1;

<<<<<<< HEAD
    nfds=epoll_wait(this->epfd,this->events,this->used,msec);
=======
    //nfds=epoll_wait(this->epfd,this->events,this->used,msec);
    nfds=epoll_wait(this->epfd,this->events,this->size,msec);
>>>>>>> develop

    if(nfds==-1)
    {
        perror("epoll_wait");
        return errno;
    }

<<<<<<< HEAD
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
=======
	if(nfds==0)
	{
		fprintf(stderr,"monitering: %d,none available\n",this->used);
	}

    for(i=0; i<nfds; i++)
    {
        epoll_event=this->events+i;
		http_event = (http_event_t*)epoll_event->data.ptr;
		//get the corresponding http_event associated with the epoll
		//event or fd
		http_event->event_handle(this,epoll_event); //callback
		if(i)
		{
			printf("%dth event triggered: %x of socket %d\n",
				i+1,epoll_event->events,http_event->listenfd);
		}
		continue;
    }
	//sleep(3);
    return 0;
}

int http_accept_handle(http_epoll_t *this,struct epoll_event *eevent)
{
	http_event_t *old_event=(http_event_t*)eevent->data.ptr;
	if(eevent->events & EPOLLERR)
	{
		fprintf(stderr,"EPOLLERR occured on listening fd %d\n",old_event->listenfd);
		return -1;
	}
	else if(eevent->events & EPOLLHUP)
	{
		fprintf(stderr,"EPOLLHUP occured on listening fd %d\n",old_event->listenfd);
		return -1;
	}

	//declaring variables for accepting new connection
    connection_t *c=NULL;
	http_event_t *new_event=NULL;
    struct epoll_event event;
    int flags;


	//run in a loop to accept all the requests(edge-triggered event)
	while(1)
	{
		c=(connection_t*)malloc(sizeof(connection_t));
		new_event=(http_event_t*)malloc(sizeof(*new_event));
		memset(c,0,sizeof(*c));
		memset(new_event,0,sizeof(*new_event));

		//must be initialize it to contain the size (in bytes) of the structure of
		//c->addr
		c->addr_len=sizeof(c->addr);
		c->read=1;

		new_event->listenfd=-1;
		new_event->data=c;
		new_event->event_handle=http_connection_handle;


		c->connfd=accept(old_event->listenfd,(struct sockaddr*)&c->addr,&c->addr_len);
		if(c->connfd==-1)
		{
			perror("accept()");//listening socket blocks,no more requests
			free(c);
			free(new_event);
			return errno;
		}
		else
		{
			flags=fcntl(c->connfd,F_GETFL,0);
			if(flags==-1)
			{
				perror("fcntl:F_GETFL");
				return -1;
			}
			flags|=O_NONBLOCK;
			if(fcntl(c->connfd,F_SETFL,flags)==-1)
			{
				perror("fcntl:O_NONBLOCK");
				return -1;
			}
			strcpy(c->ip,inet_ntoa(c->addr.sin_addr));
			c->port=c->addr.sin_port;
			printf("%s: %d\tsocket: %d\t",
					c->ip,
					c->addr.sin_port,
					c->connfd);
			event.data.ptr=new_event;
			event.events=EPOLLIN | EPOLLOUT | EPOLLET| EPOLLERR |EPOLLRDHUP;
			//event.events=EPOLLIN | EPOLLOUT | EPOLLERR |EPOLLRDHUP;
			http_epoll_add(this,c->connfd,&event);
		}
	}
>>>>>>> develop
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
<<<<<<< HEAD
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
=======
	flags=fcntl(fd,F_GETFL,0);
	if(flags==-1)
	{
		perror("fcntl F_GETFL");
		return -1;
	}
	flags|= O_NONBLOCK;
	if(fcntl(e->listenfd,F_SETFL,flags)==-1)
	{
		perror("fcntl listen socket");
		return -1;
	}


	event.data.ptr=e;
	event.events=EPOLLIN |EPOLLET | EPOLLERR | EPOLLRDHUP ;
	//event.events=EPOLLIN | EPOLLERR | EPOLLHUP ;
>>>>>>> develop
	if(http_epoll_add(this,fd,&event)!=0)
	{
		return errno;
	}
<<<<<<< HEAD
=======
	printf("Added listen socket %d to epoll instance\n",fd);
>>>>>>> develop
	return 0;
}

int http_connection_handle(http_epoll_t *this,struct epoll_event *eevent)
{
	connection_t *c=((http_event_t*)eevent->data.ptr)->data;
<<<<<<< HEAD
	if(eevent->events & EPOLLIN)
	{
		if(accept_request(c->fd,&c->request.str))
		{
			parse_request(c->request);
			//strncpy(c->path,c->request.path,sizeof(c->path));
			c->sent=0;
=======
	//char buf[MAXLINE];

	if(eevent->events & EPOLLERR)
	{
		fprintf(stderr,"EPOLLERR occured on connection fd %d\n",c->connfd);
		http_epoll_del(this,c->connfd,eevent);
		return -1;
	}
	else if(eevent->events & EPOLLHUP)
	{
		fprintf(stderr,"EPOLLHUP occured on connection fd %d\n",c->connfd);
		http_epoll_del(this,c->connfd,eevent);
		return -1;
	}
	else if(eevent->events & EPOLLRDHUP)
	{
		fprintf(stderr,"EPOLLRDHUP occured on connection fd %d\n",c->connfd);
		http_epoll_del(this,c->connfd,eevent);
		return -1;
	}
	else if(eevent->events & EPOLLIN)
	{//EPOLLIN:available for read
		if(c->read && (http_recv(c)==0))
		{
			parse_request(&c->request);
			c->sent=0;
			if(c->request.notfound)
			{//file not found
				http_connection_error_headers(c,404,"Not Found");
				http_connection_error_page(c,404);
				c->senddata=1;
				c->sendheaders=1;
				http_send(c);
				http_epoll_del(this,c->connfd,eevent);
			}
>>>>>>> develop
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
<<<<<<< HEAD
=======
					http_connection_headers(c);
>>>>>>> develop
					if((c->fd=open(c->request.path,O_RDONLY))==-1)
					{
						fprintf(stderr,"open %s error\n",c->request.path);
					}
<<<<<<< HEAD
=======
					else
					{
						setnonblock(c->fd);
					}
					c->sendheaders=1;
					c->read=0;   //we are writing to socket from now on
>>>>>>> develop
				}
			}
		}
	}
<<<<<<< HEAD
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
=======
	if(eevent->events & EPOLLOUT)
	{//EPOLLOUT
		if(c->read)
		{//reading data,not writing to it,so aborting
			printf("EPOLLOUT when reading,ignoring\n");
			return 0;
		}
		http_send(c);
		if(c->fin)
		{
			http_epoll_del(this,c->connfd,eevent);
>>>>>>> develop
		}
	}
    return 0;
}

<<<<<<< HEAD
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
=======
int http_connection_headers(connection_t *c)
{
	char buf[MAXLINE];
	sprintf(c->headers,"HTTP/1.0 200 OK\r\n");
	strcat(c->headers,"Server: ");
	strcat(c->headers,SERVER);
	strcat(c->headers,"\r\n");
	get_mime_t(buf,c->request.path);
	strcat(c->headers,"Content-Type: ");
	strcat(c->headers,buf);
	strcat(c->headers,"\r\n");
	strcat(c->headers,"Content-Length: ");
	sprintf(buf,"%ld\r\n",c->request.st.st_size);
	strcat(c->headers,buf);
	strcat(c->headers,"Host: Linux\r\n");
	strcat(c->headers,"\r\n");
	return 0;
}

int http_connection_error_page(connection_t *c,int errcode)
{
	strcat(c->headers,"<html><title>404 Not Found</title>\n"
			"<body>The request file was not found on the server"
			"</body>\n</html>\n");
	return 0;
}

int http_connection_error_headers(connection_t *c,
		int code,const char *message)
{
	sprintf(c->headers,"HTTP/1.0 %d %s\r\n",code,message);
	strcat(c->headers,"Content-Type: text/html\r\n");
	strcat(c->headers,"\r\n");
	return 0;
}

int http_request_handle(request_t *request)
{
	return 0;
}

int http_recv(connection_t *c)
{
	int nr=0;
	c->request.str=malloc(MAX_REQUEST_L);
	int size=MAX_REQUEST_L;
	while(1)
	{
		nr=read(c->connfd,c->request.str,size-1);
		if(nr==size-1)
		{
			fprintf(stderr,"%s,insufficient arrary lenth %d\n",c->request.str,size);
			return -1;
		}
		else if(nr==-1)
		{
			perror("read");
			break;
		}
	}
	return 0;
}

int http_send(connection_t *c)
{
	int n,ns=0,nr=0,l;
	//char buf[MAXLINE];
	//char buf[6291456];
	char buf[1];
	if(c->sendheaders)
	{
		l=strlen(c->headers)-c->sent;
		while(1)
		{
			n=send(c->connfd,c->headers+c->sent,l,0);
			if(n==-1)
			{//error occured
				if(errno==EAGAIN)
				{
					perror("send");
					fprintf(stderr,"socket %d would block on when sending headers\n",c->connfd);
					break;
				}
				else
				{
					perror("send");
					fprintf(stderr,"connection socket %d send error\n",c->connfd);
					return -1;
				}
			}
			else if(n>0)
			{
				c->sent+=n;
				l-=n;
			}
			else if(n==0)
			{//sending finished
				break;
			}
		}
		if(l==0)
		{
			c->sendheaders=0;
			c->sent=0;
			l=c->request.st.st_size;
			ns=0;
		}
	}
	if(c->sendheaders==0 )
	{//sending file ,not headers
		if(c->senddata)
		{//no file to send
			c->fin=1;
			return 0;
		}
		while(1)
		{//CRITICAL,using while(1) instead of while(l)
			if(lseek(c->fd,c->sent,SEEK_SET)==-1)
			{//continue after last time send
				perror("lseek");
				return -1;
			}

			nr=read(c->fd,buf,sizeof(buf));
			if(nr>0)
			{
				n=send(c->connfd,buf,nr,0);
				if(n==-1)
				{//error occured
					if(errno==EAGAIN)
					{
						fprintf(stderr,"socket %d would block when sending data\n",c->connfd);
						perror("send");
						break;
					}
					else
					{
						fprintf(stderr,"errors other than EAGAGIN occured\n");
						perror("send");
						break;
					}
				}
				else
				{//sending data without errors
					c->sent+=n;
					l-=n;

					if(n!=nr)
					{//send less data than read
						//fprintf(stderr,"sent %d, less than read %d\n",n,nr);
					}
				}
			}
			else if(nr==0)
			{//end of file
				fprintf(stderr,"End of file\n");
>>>>>>> develop
				break;
			}
			else
			{
<<<<<<< HEAD
				return -1;
			}
		}
	}
	c->sent+=ns;
	return ns;
}

=======
				//error in read data from fd wait for next epoll_wait cycle to
				//continue sending data to the socket. TO DO.
				if(errno==EAGAIN)
				{//file blocks
					fprintf(stderr,"file %d would block\n",c->fd);
					perror("read");
					break;
				}
				fprintf(stderr,"reading data from fd error\n");
			}
		}
	}
	if(c->request.st.st_size==c->sent)
	{//sending data finished
		c->fin=1;
		fprintf(stderr,"sending data to socket %d finished\n",c->connfd);
	}
	return ns;
}

int setnonblock(int fd)
{
	int flags=fcntl(fd,F_GETFL,0);
	if(flags==-1)
	{
		perror("fcntl:F_GETFL");
		return -1;
	}
	flags |= O_NONBLOCK;
	if(fcntl(fd,F_SETFL,flags)==-1)
	{
		perror("fcntl: O_NONBLOCK");
		return -1;
	}
	return 0;
}
>>>>>>> develop
