CFLAGS=-g -Wall -lpthread

httpd:httpd.o http_urldecode.o
	$(CC) -o $@ $^ $(CFLAGS)
httpd.o:httpd.c 
	$(CC) -c -o $@ $^ $(CFLAGS)
test:test.c
	$(CC) -o $@ $^ $(CFLAGS)
clean:
	rm httpd test *.o -v
