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

TO-DO:
streaming
urldecode
multiplexing socket for event-driven(signal-drive) asynchronous server
solve "bind:address already in use"
