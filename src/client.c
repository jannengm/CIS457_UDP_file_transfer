/*******************************************************************************
 *
 ******************************************************************************/

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "rudp_packet.h"
#include <time.h>

int main(int argc, char **argv){
    int sockfd, count, len;
    ssize_t bytes_read;
    struct sockaddr_in serveraddr;
    char filename[MAX_LINE], read_buf[MAX_LINE];
    rudp_packet_t *rudp_pkt;
    FILE *file;
    bool is_open;

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

    /*Initialize data packet with file request*/
    u_int32_t seq_num = 0;
    rudp_pkt = create_rudp_packet(filename, strlen(filename), &seq_num);

    /*Change packet type to SYN*/
    rudp_pkt->checksum = 0;
    rudp_pkt->type = SYN;
    rudp_pkt->checksum = calc_checksum(rudp_pkt);

    /*Stop and wait for SYN_ACK*/
    rudp_packet_t ack;
    send_and_wait(sockfd, (struct sockaddr *)&serveraddr, rudp_pkt,
                  strlen(filename) + RUDP_HEAD, &ack, NULL);

    /*Send ACK for SYN_ACK. If packet dropped, will resend ack in loop*/
    send_rudp_ack(sockfd, (struct sockaddr *) &serveraddr, &ack);

    /*Did the server locate the file?*/
    is_open = (bool)ack.data;
    if(is_open){
        fprintf(stdout, "\nServer successfully opened %s\n", filename);
    }
    else{
        fprintf(stdout, "\nServer could not locate %s\n", filename);
    }
    free(rudp_pkt);

    /*Open file write file*/
    sprintf(filename, "%s.out", filename);
    file = fopen(filename, "w+");

    if(file == NULL){
        fprintf(stdout, "\nFailed to open %s\n", filename);
        is_open = FALSE;
    }
    else{
        fprintf(stdout, "\nOpened %s\n", filename);
    }

    /*Read file from server*/
    count = 0;
    len = sizeof(struct sockaddr_in);
    while(is_open) {
        /*Receive packet from server*/
        memset(read_buf, 0, MAX_LINE);
        bytes_read = recvfrom(sockfd, read_buf, MAX_LINE,
                              0, (struct sockaddr *) &serveraddr,
                              (socklen_t *) &len);
        rudp_pkt = (rudp_packet_t *) read_buf;

        /*Print packet contents to stdout*/
        fprintf(stdout, "\nGot %d byte packet\n", (int) bytes_read);
        bool good_checksum = print_rudp_packet(rudp_pkt);

        if(good_checksum){
            fprintf(stdout, "\t|-Sending ACK for packet #%d\n", rudp_pkt->seq_num);
            send_rudp_ack(sockfd, (struct sockaddr *) &serveraddr, rudp_pkt);
        }
        else{
            fprintf(stdout, "\t|-BAD CHECKSUM\n");
            continue;
        }

        if( rudp_pkt->type == END_SEQ ) {
            break;
        }
        count += bytes_read - RUDP_HEAD;

        /*Adjust file pointer to correct location for packet*/
        fseek(file, RUDP_DATA * rudp_pkt->seq_num, SEEK_SET);

        /*Write to file*/
        fprintf(stderr, "\t|-Writing packet %d to file\n", rudp_pkt->seq_num);
        fwrite(rudp_pkt->data, 1, (size_t) (bytes_read - RUDP_HEAD), file);
    }
    fprintf(stdout, "%d total bytes received\n", count);

    /*Clean up*/
    fclose(file);
    close(sockfd);
    return 0;
}