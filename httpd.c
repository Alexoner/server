/*************************************************************************
    > File Name: httpd.c
    > Author: onerhao
    > Mail: haodu@hustunique.com
    > Created Time: Thu 18 Jul 2013 11:58:40 PM EDT
 ************************************************************************/

/**
 * @1 Read all the data in socket before write into it
 * @2 Socket cannot be lseeked
 * @3 Function read and recv blocks when no data is in the socket
 * @4 Select() and poll() and be used for multiplexing,
 * solving blocking problems
 * @5 When read() or recv() return a value no bigger than 0,jump out
 * of the loop
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <libgen.h>
#include <time.h>
#include <getopt.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "httpd.h"
#include "http_epoll.h"
#include "http_urldecode.h"


typedef enum
{
    HTTP_VERSION_1_1,
    HTTP_VERSION_1_0,
    HTTP_VERSION_0_9,
    HTTP_VERSION_UNKNOWN
} http_version_t;

typedef enum
{
    HTTP_METHOD_GET,
    HTTP_METHOD_POST,
    HTTP_METHOD_HEAD,
    HTTP_METHOD_UNKNOWN
} http_method_t;

/* If it is determined via the request it is in this struct */
/*typedef struct {
	http_version_t http_version;
	http_method_t http_method;

	unsigned int status_code;

	char *uri;
	char *query_ptr; //Points to first ? in uri
	char *filepath;
	char *extension_ptr; //Points to . in filepath
	struct stat *filestats;

	char *host;
	unsigned int keep_alive;

	buffer *unparsed_data;

	int fd;
} request;
*/

extern http_epoll_t http_epoll;

char root_dir[MAX_PATH_L];
char *cgi_dir[2]= {"cgi-bin","htdocs"};


/**********************************************************************/
/* This function starts the process of listening for web connections
 * on a specified port.  If the port is 0, then dynamically allocate a
 * port and modify the original port variable to reflect the actual
 * port.
 * Parameters: pointer to variable containing the port to connect on
 * Returns: the socket */
/**********************************************************************/
int startserver(int *listenfd,int *port)
{
    int namelen=0;
    struct sockaddr_in name;//IPv4 socket (name)address
    //httpd=socket(AF_INET,SOCK_STREAM,0);
    *listenfd=socket(AF_INET,SOCK_STREAM,0);
    //address family:IPv4 protocol;
    if(*listenfd==-1)
    {
        error_die("socket");
    }
    name.sin_family=AF_INET;
    name.sin_port=htons(*port);
    name.sin_addr.s_addr=htonl(INADDR_ANY);
    /*Internet address.
     *Address to accept any incoming messages. */
    if(bind(*listenfd,(struct sockaddr*)&name,sizeof(name))<0)
    {
        //assigning a name to a socket
        error_die("bind");
    }
    if(*port==0)//dynamically allocating a port
    {
        if(getsockname(
                    *listenfd,
                    (struct sockaddr*)&name,
                    (unsigned *)&namelen)==-1)
        {
            error_die("getsockname");
        }
        *port=ntohs(name.sin_port);
        printf("Reassigning port to %d\n",*port);
    }
    if(listen(*listenfd,LISTENQ)<0)
    {
        error_die("listen");
    }
    return *listenfd;
}

int serve_forever(int listenfd)
{
    int connfd;
    struct sockaddr_in connaddr;//IPv4 socket address for connection
    char *request=NULL;
    int len=sizeof(connfd);
	signal(SIGPIPE,SIG_IGN);//ignore the SIGPIPE signal
    while(1)
    {
        connfd=accept(listenfd,
                      (struct sockaddr*)&connaddr,
                      (unsigned*)&len);
        if(connfd==-1)
        {
            error_die("accept");
        }
        accept_request(connfd,&request);
        process_request(connfd,request);
        close(connfd);
    }
    if(request)
    {
        free(request);
    }
    return 0;
}

int readc(int fd,void* buf,size_t count);

int readline(int fd,char *buf,size_t count)
{
    int i=0,rc;
    char c='\0';
    for(i=0; i<count-1; i++)
    {
        rc=recv(fd,&c,sizeof(c),0);
        if(rc==1)
        {
            buf[i]=c;
            if(c=='\n')
            {
                break;//newline
            }
        }
        else if(rc==0)
        {
            //have read all bytes
            i--;
            break;
        }
        else
        {
            return -1;//error
        }
    }
    buf[i+1]='\0';//null terminate
    //printf("%s",buf);
    return i+1;//i+1 bytes read
}

int accept_request(int connfd,char **request)
{
    *request=(char*)malloc(sizeof(char)*MAXLINE);
    int n=0;
    //n=recv(connfd,*request,MAXLINE,0);
    n=readline(connfd,*request,MAXLINE);
    if (n)
    {
        return 1;
    }
    else
    {
        free(*request);
        return 0;
    }
}

int process_request(int connfd,const char *request)
{
    char ns;
    char method[MAX_METHOD_L];
    char url[MAX_URL_L];
    char path[MAX_PATH_L],buf[MAX_PATH_L];
    char version[MAX_VERSION_L];
    char *s;
    struct stat st;//file status
    char query_string[MAXLINE];//parameters of GET request
    int i,cgi=0;
    if(!request)
        return -1;
    if((ns=sscanf(request,"%s %s %s",method,buf,version))!=3)
    {
        bad_request(connfd);
    }
	urldecode(buf,url);
    printf("%s",request);
	//printf("%s\n",url);
    //process for different methods
    if(!strcasecmp(method,"GET"))
    {
        //GET method
        if((s=strchr(url,'?')))
        {
            //is a cgi program
            cgi=1;
            //sprintf(path,"%s",cgi_path);
            //strncat(path,url,s-url);
            sscanf(url,"%[^?]",buf);//file path
            sprintf(path,"%s%s",root_dir,buf);//prefix of its relative path
            sscanf(url,"%*[^?]?%s",query_string);//query string parameters
        }
        else
        {
            //sscanf(url,"%s",buf);
            strcpy(buf,url);
            sprintf(path,"%s%s",root_dir,buf);
            for(i=0; i<MAX_CGI_DIR_N; i++)
            {
                if((s=strstr(url,cgi_dir[i])))
                {
                    if(s[-1]=='/'&&s[strlen(cgi_dir[i])]=='/')
                    {
                        //a cgi program,no arguments passed
                        cgi=1;
                        break;
                    }
                }
            }
            /*if(i==MAX_CGI_DIR)
            {
                //not a cgi program
                if(path[strlen(path)-1]=='/')
                {
                    //path is a directory
                    strcat(path,"index.html");
                }
            }*/
        }
    }
    else if(!strcasecmp(method,"POST"))
    {
        //POST method
        cgi=1;
        //sscanf(url,"%s",buf);
        strcpy(url,buf);
		printf("url: %s\n",buf);
        sprintf(path,"%s%s",root_dir,buf);

        //for debug
        //for(int i=0;(i=readline(connfd,buf,strlen
    }
    else
    {
        //other methods
        unimplemented(connfd);
        return -1;
    }

    //file status error
    if(stat(path,&st)==-1)
    {
        perror("stat ");
        getcwd(buf,sizeof(buf));
        printf("file is %s\n",path);
        not_found(connfd,path);
    }
    else
    {
        //if((st.st_mode & S_IXUSR)||
        //		(st.st_mode & S_IXGRP)||
        //		 (st.st_mode & S_IXOTH))

        if(!cgi)
        {
            //not a cgi program
            //directory listing
            if(S_ISDIR(st.st_mode))
            {
                //foo(connfd);
                dirlist(connfd,url,path);
            }
            else
            {
                serve_static(connfd,path);
            }
        }
        else
        {
            //serve as a cgi program
            serve_dynamic(connfd,path,method,query_string);
        }
    }
    return 0;
}

void send_headers(int connfd,const char *mimetype,size_t size)
{
    char buf[MAXLINE];
    strcpy(buf,"HTTP/1.0 200 OK\r\n");
    strcat(buf,SERVER);
    strcat(buf,"\r\nContent-Type: ");
    strcat(buf,mimetype);
    strcat(buf,"\r\n");
    send(connfd,buf,strlen(buf),0);
}

void end_headers(int connfd)
{
    //send(connfd,(void*)"\r\n",sizeof("\r\n"),0);//error to use sizeof!!
    send(connfd,(void*)"\r\n",strlen("\r\n"),0);
    //a blank line seperate headers from the content
}

int send_content(int connfd,const char *mime,const char* path)
{
	char str[MAX_MIME_L];
	if(sscanf(mime,"%[^/]",str)<=0)
		return -1;
	if(!strcmp(str,"text"))
	{//text file
		send_text(connfd,path);
	}
	else
	{//binary file
		send_binary(connfd,path);
	}
	return 0;
}

int send_text(int connfd,const char *path)
{
	char buf[MAXLINE];
	FILE *fp=fopen(path,"rt");
	while(!feof(fp)&&!ferror(fp))
	{
		fgets(buf,sizeof(buf),fp);
		if(!feof(fp)&&!ferror(fp))
		{
			if((send(connfd,buf,strlen(buf),0))==-1)
			{
				break;//jump out of the loop if error occurs
			}
		}
	}
	return 0;
}

int send_binary(int connfd,const char *path)
{
	int n;
    char buf[MAXLINE];
    FILE *fp=fopen(path,"rb");
    while(!feof(fp)&&!ferror(fp))
    {
        //fgets(buf,sizeof(buf),fp);
		n=fread(buf,sizeof(char),sizeof(buf)/sizeof(char),fp);
        if(!feof(fp)&&!ferror(fp))
        {
            send(connfd,buf,n*sizeof(char),0);
        }
    }
	return 0;
}

int foo(int connfd)
{
    char buf[MAXLINE];
    clear_socket(connfd);
    send_headers(connfd,"text/html",0);
    end_headers(connfd);
    sprintf(buf,
            "<!DOCTYPE html>\r\n"
            "<html>\r\n"
            "<h1>Index of </h1>\r\n"
            "</html>\r\n");
    send(connfd,buf,strlen(buf),0);
    return 0;
}

int dirlist(int connfd,const char *url,const char *path)
{
    DIR *dp;
    struct dirent *entry;
    struct stat st;
    char buf[MAXLINE],url2[MAX_URL_L];

    clear_socket(connfd);//clear the socket

    if((dp=opendir(path))==NULL)
    {
        perror("opendir");
        return -1;
    }
    //write the html headers into the socket
    send_headers(connfd,"text/html",0);
    end_headers(connfd);
    chdir(path);

	strcpy(url2,url);
    //generate the html table,and send the html file
    sprintf (buf,
             "<!DOCTYPE html>\r\n"
             "<html>\r\n"
             "<head>\r\n"
             "<title>Index of %s</title>\r\n"
             "<style type=\"text/css\"></style>\r\n"
             "<script></script>\r\n"
             "</head>\r\n"
             "<body>\r\n"
             "<h1>Index of %s</h1>\r\n"
             "<table>\r\n"
             "<tbody>\r\n"
             "<tr>\r\n"
             "<th><a href=\"?C=N;O=D\">Name</a></th>\r\n"
             "<th><a href=\"?C=M;O=A\">Last modified</a></th>\r\n"
             "<th><a href=\"?C=S;O=A\">Size</a></th>\r\n"
             "<th><a href=\"?C=D;O=A\">Description</a></th>\r\n"
             "</tr>\r\n"
             "<tr><th colspan=\"5\"></th></tr>\r\n"
             "<tr>\r\n"
             "<td><a href=\"%s\">Parent Directory</a></td>\r\n"
             "<td>&nbsp;</td>\r\n"
             "<td align=\"right\">  -</td>\r\n"
             "<td>&nbsp;</td>\r\n"
             "</tr>\r\n",
			url,
			url,
			dirname((char*)url2)
            );
    if((send(connfd,buf,strlen(buf),0))==-1)
    {
        perror("send");
    }

    while((entry=readdir(dp)))
    {
        if(lstat(entry->d_name,&st)==0)
        {
            //generate html element
            if(!strcmp(entry->d_name,".")||!strcmp(entry->d_name,".."))
            {
                //the current directory and its parent directory,skip
                continue;
            }
            else
            {
                char buf[MAXLINE];
				char directory[2];
				memset(directory,0,sizeof(directory));
				if(S_ISDIR(st.st_mode))
				{
					sprintf(directory,"/");
				}
				strcpy(url2,url);
				if(url2[strlen(url2)-1]!='/')
				{
					strcat(url2,"/");
				}
               sprintf(buf,
                        "<tr>\n"
                        "<td>\n"
                        "<a href=\"%s%s\">%s%s</a>\n"
                        "</td>\n"
                        "<td align=\"right\">%s</td>\n"
                        "<td align=\"right\">%zu</td>\n"
                        "<td>&nbsp;</td>\n"
                        "</tr>\n",
                        url2,entry->d_name,entry->d_name,directory,
						ctime(&st.st_mtime),
						st.st_size);
                if((send(connfd,buf,strlen(buf),0))==-1)
                {
                    perror("send");
                    break;
                }
            }
        }
    }
    sprintf(buf,
            "</tbody>\n"
            "</table>\n"
            "<address>NHW Web/0.0.1 (Ubuntu) Server</address>\n"
            "</body>\n"
            "</html>\n");
    if((write(connfd,buf,strlen(buf)))==-1)
    {
        perror("write");
    }
    chdir(root_dir);//chdir back to the root directory
    return 0;
}

int serve_dynamic( int connfd, const char *path,
                   const char *method, const char *query_string)
{
    char buf[MAXLINE],str[MAXLINE],c;
    int cgi_output[2],cgi_input[2];
    pid_t pid;
    int status;
    int i,nr=0,content_length=-1;
    if(strcasecmp(method,"POST")==0)
    {
        //locate "Content-Length:"
        while((nr=readline(connfd,buf,sizeof(buf)))>0&&strcmp(buf,"\r\n"))
        {
            //read all the headers
            if((strstr(buf,CONTENT_LENGTH_S)))
            {
                sscanf(buf,"%*[^0-9]%[0-9]",str);//get the CONTENT_LENGTH
                //printf("? %",str);
                content_length=atoi(str);
            }
        }
        if(content_length==-1)
        {
            bad_request(connfd);
            return -1;
        }
    }
    else
    {
        //method is GET
        read_headers(connfd,buf,sizeof(buf));//read & discard headers
    }

    //CGI programs will send"Content-Type:..."
    sprintf(buf,"%s","HTTP/1.0 200 OK\r\n");
    send(connfd,buf,strlen(buf),0);

    if(pipe(cgi_output)<0||pipe(cgi_input)<0)
    {
        //create pipe for communication between processes
        cannot_execute(connfd);
        return -1;
    }

    //create child process
    if((pid=fork())<0)
    {
        cannot_execute(connfd);
        return -1;
    }

    if(pid==0)//child:CGI script
    {
        char method_env[MAX_NAME_VALUE];
        char query_env[MAX_NAME_VALUE];
        char content_len_env[MAX_NAME_VALUE];

        dup2(cgi_input[0],0);//duplicate it to stdin
        dup2(cgi_output[1],1);//stdout
        close(cgi_input[1]);//close the writing end
        close(cgi_output[0]);

        //set the environment variable
        sprintf(method_env,"REQUEST_METHOD=%s",method);
        putenv(method_env);

        if(strcasecmp(method,"GET")==0)
        {
            sprintf(query_env,"QUERY_STRING=%s",query_string);
            putenv(query_env);
        }
        else
        {
            //POST request
            sprintf(content_len_env,"CONTENT_LENGTH=%d",content_length);
            putenv(content_len_env);
        }

        execl(path,path,NULL);
        exit(0);//exit the child process
    }
    else//parent
    {
        close(cgi_output[1]);//close writting end
        close(cgi_input[0]);//close reading end
        if(strcasecmp(method,"POST")==0)
        {
            for(i=0; i<content_length; i++)
            {
                recv(connfd,&c,sizeof(c),0);
                if(write(cgi_input[1],&c,sizeof(c))==-1)
				{
					break;
				}
                //putchar(c);
            }
        }
        while(read(cgi_output[0],&c,1)>0)
        {
            if((send(connfd,&c,1,0))==-1)
			{
				break;
			}
        }
        close(cgi_output[0]);
        close(cgi_input[1]);
        waitpid(pid,&status,0);
        return 0;
    }
}

int serve_static(int connfd,const char *path)
{
	int size;
	struct stat st;
    char buf[MAXLINE];

	clear_socket(connfd);
	if(stat(path,&st)==-1)
	{
		not_found(connfd,path);
		return -1;
	}
	get_mime_t(buf,path);
	size=st.st_size;
    send_headers(connfd,buf,size);
    end_headers(connfd);
    send_content(connfd,buf,path);
    return 0;
}

int clear_socket(int connfd)
{
    fd_set rfds;
    int maxfds;
    struct timeval tv;
    int nc,ret;
    char c;

    FD_ZERO(&rfds);
    FD_SET(connfd,&rfds);
    maxfds=connfd+1;
    tv.tv_sec=0;
    tv.tv_usec=0;

    while(1)
    {
        ret=select(maxfds,&rfds,NULL,NULL,&tv);
        if(ret<0)
        {
            if(errno==EINTR)
            {
                continue;
                return -1;
            }
            else
            {
                //perror("select");
                return -1;
            }
        }
        if(FD_ISSET(connfd,&rfds))
        {
            //connfd socket ready to be read
            if((nc=read(connfd,&c,sizeof(c)))>0)
            {
                //putchar(c);
            }
            else
            {
                /*jump out of the loop,
                 * or program may block in this loop somehow
                 */
                break;
            }
        }
        else
        {
            //select timeout
            //perror("select");
            return -1;
        }
    }
    return 0;
}

int read_headers(int connfd,char *buf,size_t count)
{
    int nc;
    while((nc=readline(connfd,buf,count))&&strcmp(buf,"\r\n"));
    return 0;
}

char *get_mime_t(char *type,const char *path)
{
	int fd[2];
	pid_t pid;
	char buf[MAX_MIME_L];
	pipe(fd);
	if((pid=fork())<0)
	{
		perror("fork");
		return NULL;
	}
	if(pid==0)
	{//child process
		dup2(fd[1],STDOUT_FILENO);
		execl("/usr/bin/file","file","-L","-i",path,NULL);
		close(fd[1]);
		return NULL;
	}
	else
	{//parent process
		read(fd[0],buf,sizeof(buf));
		close(fd[0]);
		sscanf(buf,"%*[^ ] %[^;]",type);
		waitpid(pid,NULL,0);
		return type;
	}
}

void error_die(const char *s)
{
    perror(s);
    exit(1);
}

void send_error(int connfd,int code,const char *message)
{
    char buf[MAXLINE];
    sprintf(buf,"HTTP/1.0 %d %s\r\n",code,message);
    strcat(buf,"Content-Type: text/html\r\n");
    send(connfd,buf,strlen(buf),0);
    end_headers(connfd);
}

void unimplemented(int connfd)
{
    char buf[MAXLINE];
    clear_socket(connfd);
    send_error(connfd,501,"Unimplemented");

    sprintf(buf,"<html><head><title>Method not implemented</title></head></html>");
    send(connfd,buf,strlen(buf),0);
}

void not_found(int connfd,const char *path)
{
    char buf[MAXLINE];
    clear_socket(connfd);
    send_error(connfd,404,"Not found");

    sprintf(buf,"<html><title>404 Not Found</title>\r\n");
    send(connfd,buf,strlen(buf),0);
    sprintf(buf,"<body>The requested file %s was not found on this server</body>\r\n</html>\r\n",path);
    send(connfd,buf,strlen(buf),0);
}

void cannot_execute(int connfd)
{
    char buf[MAXLINE];
    clear_socket(connfd);
    send_error(connfd,500,"Internal Server Error");
    sprintf(buf,"<p>The server encountered an internal error or misconfiguration and was unable to complete your request.</p>\r\n");
    send(connfd,buf,strlen(buf),0);
}

void bad_request(int connfd)
{
    char buf[MAXLINE];
    clear_socket(connfd);
    send_error(connfd,400,"Bad Request");
    sprintf(buf,"<p>Your browser has sent a bad request.</p>\r\n");
    send(connfd,buf,strlen(buf),0);
}

int main(int argc,char **argv)
{
    int listenfd;//file descriptor for listening socket
    int port=DEFAULT_PORT;//listening port
    getcwd(root_dir,sizeof(root_dir));

    listenfd=startserver(&listenfd,&port);
    printf("NHW httpd is running on port %d\n",port);
    serve_forever(listenfd);

    close(listenfd);
    return 0;
}
