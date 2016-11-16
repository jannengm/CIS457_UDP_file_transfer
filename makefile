#Makefile

make: server client clean

server: rudp_packet.o
	gcc -Wall rudp_packet.o src/server.c -o bin/server

client: rudp_packet.o
	gcc -Wall rudp_packet.o src/client.c -o bin/client

rudp_packet.o:
	gcc -Wall -c src/rudp_packet.c src/rudp_packet.h

clean:
	rm *.o
