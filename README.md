server
======

TCP/IP server,including http only for now

IMPLEMENTED:
serve static files
serve dynamic files
error process,error code
directory listing
mime type
non-blocking read all data in socket
threaded processing per request
CGI

TO-DO:
streaming
urldecode
multiplexing socket for event driven asynchronous server
solve "bind:address already in use"
