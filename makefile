all : client server

client : readline.o client.o user.o sha256_lib.o
	gcc -g -o $@ $^

server : readline.o server.o user.o
	gcc -g -o $@ $^

client.o : client.c option.h
	gcc -g -c $<

server.o : server.c option.h
	gcc -g -c $<

%.o : %.c %.h
	gcc -g -c $<

clean :
	rm *.o client server
