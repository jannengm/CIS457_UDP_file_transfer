#Makefile

make: server client clean

server: rudp_packet.o window.o
	gcc -Wall rudp_packet.o window.o src/server.c -o bin/server -pthread

client: rudp_packet.o window.o
	gcc -Wall rudp_packet.o window.o src/client.c -o bin/client

rudp_packet.o:
	gcc -Wall -c src/rudp_packet.c src/rudp_packet.h

window.o:
	gcc -Wall -c src/window.c src/window.h src/rudp_packet.h

clean:
	rm *.o
	rm src/*.gch
