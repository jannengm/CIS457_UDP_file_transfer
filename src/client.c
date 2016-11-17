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
    char filename[MAX_LINE], read_buf[MAX_LINE];
    rudp_packet_t *rudp_pkt;
    FILE *file;

    /*Check command line arguments*/
    if(argc != 3 && argc != 4) {
        fprintf(stderr, "Usage: %s [Port] [IPv4 address] "
                "[(optional) filename]\n", argv[0]);
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
    if(argc != 4) {
        fprintf(stdout, "Enter filename: ");
        fscanf(stdin, "%s", filename);
        filename[strlen(filename)] = '\0';
    }
    else{
        memset(filename, 0, MAX_LINE);
        memcpy(filename, argv[3], strlen(argv[3]));
    }

    /*Send file name to server*/
    fprintf(stdout, "Requesting %s from server...\n", filename);
    sendto(sockfd, filename, strlen(filename), 0,
           (struct sockaddr*)&serveraddr, sizeof(struct sockaddr_in) );

    /*Open file write file*/
    sprintf(filename, "%s.out", filename);
    file = fopen(filename, "w+");

    /*Read file from server*/
    count = 0;
    len = sizeof(struct sockaddr_in);
    do{
        memset(read_buf, 0, MAX_LINE);
        bytes_read = recvfrom(sockfd, read_buf, MAX_LINE,
                              0, (struct sockaddr*)&serveraddr,
                              (socklen_t *) &len);
        rudp_pkt = (rudp_packet_t *)read_buf;

        fprintf(stdout, "Got %d byte packet\n", (int)bytes_read);
        fprintf(stdout, "\tSequence number: %d\n", rudp_pkt->seq_num);
        fprintf(stdout, "\tChecksum: 0x%04x ", rudp_pkt->checksum);
        fprintf(stdout, "(%s)\n", ((calc_checksum(rudp_pkt) == 0) ?
                                  "correct" : "incorrect"));
        count += bytes_read - RUDP_HEAD;

        //TODO:Write data to file
        fwrite(rudp_pkt->data, 1, bytes_read - RUDP_HEAD, file);

    }while(rudp_pkt->seq_num != END_SEQ);
    fprintf(stdout, "%d total bytes received\n", count);

    /*Clean up*/
    fclose(file);
    close(sockfd);
    return 0;
}