/*************************************************************************
    > File Name: httpd.h
    > Author: onerhao
# mail: onerhao@gmail.com
    > Created Time: Wed 30 Oct 2013 11:09:47 AM CST
 ************************************************************************/

#ifndef HTTPD_H
#define HTTPD_H

#include <sys/types.h>
#include <sys/stat.h>

#define LISTENQ					5
#define DEFAULT_PORT			9117
#define ROOT_DIR				""
#define MAX_NAME_VALUE			255
#define MAXLINE					65535
#define MAX_REQUEST_L           1024
#define MAX_METHOD_L			128
#define MAX_URL_L				512
#define MAX_PATH_L				512
#define MAX_HEADERS_L           1024
#define MAX_VERSION_L			16
#define MAX_CGI_DIR_N			2
#define MAX_MIME_L				255
#define SERVER					"HTTP/0.1 C/99"
#define CONTENT_LENGTH_S		"Content-Length:"

#define DEBUG					1

typedef struct request_s
{
    char *str;
    char method[MAX_METHOD_L];
    char url[MAX_URL_L];
    char version[MAX_VERSION_L];
    char path[MAX_PATH_L];
    struct stat st;
    char *arg;
    int cgi:1;
    int notfound:1;
    int notimplemented:1;
    int directory:1;
}request_t;

//initialize the server
int startserver(int *listenfd,int *port);

//Handle request until shutdown
int serve_forever(int listenfd);

//Handle a request then exit
int handle_request(int connfd);

//threaded request handling function
void *thread_handle_request(void *fd);

//read a character from the stdio
int readc(int fd,void *buf,size_t count);

//read a line from the stdio
int readline(int fd,char *buf,size_t maxlen);

//accept a request from the socket
int accept_request(int connfd,char **request);

int parse_request(request_t *request);

void verify_request(const char *request);

//process the request
int process_request(int connfd,const char *request);

/*Sends and logs a complete error reply to the client.
 * The numeric code specifies the HTTP error code
 */
int send_error(int connfd,int error_code,const char* message);

//Writes a specific HTTP header to the output stream
int send_headers(int connfd,const char *mimetype,size_t size);

//Sends a blank line, indicating the end of the HTTP headers in the response
int end_headers(int connfd);

//send the content of a file
int send_content(int connfd,const char *mime,const char *path);

//send binary file
int send_binary(int connfd,const char *path);

//send text file
int send_text(int connfd,const char *path);

//directory listing
int dirlist(int connfd,const char *url,const char *path);

//execute it as cgi script
int serve_dynamic(int connfd, const char* path,
                  const char* method, const char* query_string);

//serve it as a file
int serve_static(int connfd,const char *path);

//read and discard data in the socket
int clear_socket(int connfd);

//read and discards headers
int read_headers(int connfd,char *buf,size_t count);

//get mime file types
char *get_mime_t(char *type,const char *path);

//print error and exit
void error_die(const char *s);

//send error response headers.
int send_error(int connfd,int code,const char *message);

//unimplemented method
void unimplemented(int connfd);

//file not found
void not_found(int connfd,const char *path);

//file cannot be executed
void cannot_execute(int connfd);

//bad request
void bad_request(int connfd);

#endif
