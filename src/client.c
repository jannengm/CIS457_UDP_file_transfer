/*******************************************************************************
 *
 ******************************************************************************/

#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "rudp_packet.h"

int main(int argc, char **argv){
    int sockfd, count, len;
    ssize_t bytes_read;
    struct sockaddr_in serveraddr;
    char filename[MAX_LINE], read_buf[RUDP_DATA];

    /*Check command line arguments*/
    if(argc != 3) {
        fprintf(stderr, "Usage: %s [Port] [IPv4 address]\n", argv[0]);
        exit(1);
    }

    /*Create UDP socket*/
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0){
        printf("There was an error creating the socket\n");
        exit(1);
    }

    /*Create server address and port*/
    serveraddr.sin_family=AF_INET;
    serveraddr.sin_port = htons( (uint16_t)atoi(argv[1]) );
    serveraddr.sin_addr.s_addr = inet_addr(argv[2]);

    /*Prompt user for file name*/
    fprintf(stdout, "Enter filename: ");
    fscanf(stdin, "%s", filename);
    filename[strlen(filename)] = '\0';

    /*Send file name to server*/
    fprintf(stdout, "Requesting %s from server...\n", filename);
    sendto(sockfd, filename, strlen(filename), 0,
           (struct sockaddr*)&serveraddr, sizeof(struct sockaddr_in) );

    /*Read file from server*/
    count = 0;
    len = sizeof(struct sockaddr_in);
    do{
        bytes_read = recvfrom(sockfd, read_buf, RUDP_DATA, 0,
                                    (struct sockaddr*)&serveraddr,
                                    (socklen_t *) &len);
        fprintf(stdout, "Got %d byte packet\n", (int)bytes_read);
        count += bytes_read;

        //TODO:Write data to file

    }while(bytes_read == RUDP_DATA);
    fprintf(stdout, "%d total bytes received\n", count);

    /*Clean up*/
    close(sockfd);
    return 0;
}