/*******************************************************************************
 *
 ******************************************************************************/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#define PACKET_SIZE 1024
#define MAX_LINE 1024

/*******************************************************************************
 * Server main method. Expects a port number as a command line argument
 *
 * @param argc
 * @param argv
 * @return
 ******************************************************************************/
int main(int argc, char **argv){
    int sockfd, len;
    struct sockaddr_in serveraddr, clientaddr;
    char filename[MAX_LINE];
    FILE *file;

    /*Check command line arguments*/
    if(argc != 2){
        fprintf(stderr, "Usage: %s [Port]", argv[0]);
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

    /*Get file name from client*/
    len = sizeof(struct sockaddr_in);
    int n = (int) recvfrom(sockfd, filename, MAX_LINE, 0, (struct sockaddr*)&clientaddr,
                           (socklen_t *) &len);
    printf("Received %d characters\n", n);
    filename[n] = '\0';
    fprintf(stdout, "Requested file: %s\n", filename);

    /*Attempt to open the file*/
    file = fopen(filename, "r");
    if(file == NULL){
        fprintf(stderr, "Could not locate %s\n", filename);
        close(sockfd);
        exit(1);
    }

    fprintf(stdout, "Successfully opened %s\n", filename);

    /*Clean up*/
    fclose(file);
    close(sockfd);
    return 0;
}