server
======

TCP/IP server,including http only for now

IMPLEMENTED:
serve static files
serve dynamic files
error process,error code
directory listing
mime type
multiplexing socket for event-driven(signal-drive) asynchronous server(epoll)
threaded processing per request
CGI processing
urldecode

TO-DO:
streaming
email proxy
proxy agent(google app engine)
solve "bind:address already in use"
