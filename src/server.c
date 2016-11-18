/*******************************************************************************
 *
 ******************************************************************************/

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "rudp_packet.h"
#include "window.h"
#include <pthread.h>

struct thread_arg_t{
    window_t * window;
    int sockfd;
};

typedef struct thread_arg_t thread_arg_t;

void send_file(int sockfd, struct sockaddr* clientaddr, FILE *file);
void * get_acks(void * arg);

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
//    unsigned char read_buf[RUDP_DATA];
    FILE *file;
//    rudp_packet_t *rudp_pkt;

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
    send_file(sockfd, (struct sockaddr *)&clientaddr, file);

    close(sockfd);
//    return 0;
    sleep(10);
    exit(0);
}

/*******************************************************************************
 *
 * @param sockfd
 * @param clientaddr
 * @param file
 ******************************************************************************/
void send_file(int sockfd, struct sockaddr* clientaddr, FILE *file){
    int count = 0;
    ssize_t bytes_read;
    unsigned char read_buf[MAX_LINE];
    rudp_packet_t *rudp_pkt;
    pthread_t child;
    thread_arg_t arg;


    /*Detach thread to listen for ACKs*/
    arg.sockfd = sockfd;
    if( pthread_create(&child, NULL, get_acks, &arg) != 0) {
        printf("Failed to create thread\n");
        exit(1);
    }
    pthread_detach(child);

    while(!feof(file) && !ferror(file)){
        /*Read up to RUDP_DATA bytes from file*/
        bytes_read = fread(read_buf, 1, RUDP_DATA, file);
        if(ferror(file)){
            fprintf(stderr, "File read error\n");
            break;
        }

        /*If bytes successfully read in, send RUDP packet*/
        if(bytes_read > 0){
            /*Create a new RUDP packet*/
            rudp_pkt = create_rudp_packet(read_buf, (size_t)bytes_read);

            /*If file reached EOF, flag as the last packet*/
            if(feof(file)) {
                rudp_pkt->type = END_SEQ;
                rudp_pkt->checksum = 0;
                rudp_pkt->checksum = calc_checksum(rudp_pkt);
            }

            /*Send the RUDP packet*/
            sendto(sockfd, rudp_pkt, (size_t)(RUDP_HEAD + bytes_read), 0,
                   clientaddr, sizeof(struct sockaddr_in));

            /*Free RUDP packet*/
            free(rudp_pkt);

            /*Send data to client*/

            /*Print the number of bytes read in to stdout*/
            fprintf(stdout, "Read %d bytes\n", (int)bytes_read);
            count += bytes_read;
        }
    }
    printf("%d total bytes read\n", count);

    /*Clean up*/
    fclose(file);
}

/*******************************************************************************
 *
 * @param arg
 * @return
 ******************************************************************************/
void * get_acks(void * arg){
    unsigned char buffer[MAX_LINE];
    struct sockaddr_in clientaddr;
    int buf_len, len;
    int sockfd = ((thread_arg_t *)arg)->sockfd;
//    window_t * window = ((thread_arg_t *)arg)->window;

    buf_len = (int) recvfrom(sockfd, buffer, MAX_LINE, 0,
                             (struct sockaddr*)&clientaddr,
                             (socklen_t *)&len);

    if( ((rudp_packet_t *)buffer)->type == ACK ){
        fprintf(stdout, "Received %d byte acknowledgement for packet %d\n",
                buf_len, ((rudp_packet_t *)buffer)->seq_num);
    }

    return NULL;
}