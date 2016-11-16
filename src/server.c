/*******************************************************************************
 *
 ******************************************************************************/

#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "rudp_packet.h"

/*******************************************************************************
 * Server main method. Expects a port number as a command line argument
 *
 * @param argc
 * @param argv
 * @return
 ******************************************************************************/
int main(int argc, char **argv){
    int sockfd, len;
    ssize_t bytes_read;
    struct sockaddr_in serveraddr, clientaddr;
    char filename[MAX_LINE];
    unsigned char read_buf[RUDP_DATA];
    FILE *file;

    /*Check command line arguments*/
    if(argc != 2){
        fprintf(stderr, "Usage: %s [Port]\n", argv[0]);
        exit(1);
    }

    /*Create UDP socket*/
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0){
        printf("There was an error creating the socket\n");
        exit(1);
    }

    /*Set up server address and port*/
    serveraddr.sin_family=AF_INET;
    serveraddr.sin_port = htons( (uint16_t)atoi(argv[1]) );
    serveraddr.sin_addr.s_addr = INADDR_ANY;

    /*Bind server address and port to the socket*/
    bind(sockfd, (struct sockaddr*)&serveraddr, sizeof(struct sockaddr) );

    /*Listen for up to 10 clients*/
    listen(sockfd, 10);

    /*Get file name from client*/
    len = sizeof(struct sockaddr_in);
    bytes_read = recvfrom(sockfd, filename, MAX_LINE, 0,
                                (struct sockaddr*)&clientaddr,
                                (socklen_t *)&len);
    printf("Received %d characters\n", (int)bytes_read);
    filename[bytes_read] = '\0';
    fprintf(stdout, "Requested file: %s\n", filename);

    /*Attempt to open the file*/
    file = fopen(filename, "r");
    if(file == NULL){
        fprintf(stderr, "Could not locate %s\n", filename);
        close(sockfd);
        exit(1);
    }

    fprintf(stdout, "Successfully opened %s\n", filename);

    /*Read in file from disk*/
    int count = 0;
    while(!feof(file) && !ferror(file)){
        bytes_read = fread(read_buf, 1, RUDP_DATA, file);
        if(ferror(file)){
            fprintf(stderr, "File read error\n");
            break;
        }
        if(bytes_read > 0){
            //TODO: create RUDP packet and send to client

            /*Send data to client*/
            sendto(sockfd, read_buf, (size_t) bytes_read, 0,
                   (struct sockaddr*)&clientaddr, sizeof(struct sockaddr_in));

            /*Print the number of bytes read in to stdout*/
            fprintf(stdout, "Read %d bytes\n", (int)bytes_read);
            count += bytes_read;
        }
    }
    printf("%d total bytes read\n", count);

    /*Clean up*/
    fclose(file);
    close(sockfd);
    return 0;
}