server
======

TCP/IP server,including http only for now

IMPLEMENTED:
serve static files
serve dynamic files
error process,error code
directory listing
mime type
multiplexing socket file descriptor
threaded processing per request
CGI processing
urldecode

TO-DO:
streaming
multiplexing socket for event-driven(signal-drive) asynchronous server(epoll)
solve "bind:address already in use"
