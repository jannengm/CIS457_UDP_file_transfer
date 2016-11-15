#Makefile

make: server client

server:
	gcc -Wall src/server.c -o bin/server

client:
	gcc -Wall src/client.c -o bin/client
