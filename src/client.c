/*******************************************************************************
 *
 ******************************************************************************/

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "rudp_packet.h"
#include "cache_list.h"
#include <time.h>

void write_cache(cache_list_t * cache, u_int8_t * prev_pkt, FILE * file);

int main(int argc, char **argv){
    int sockfd, count, len, cycle;
    ssize_t bytes_read;
    struct sockaddr_in serveraddr;
    char filename[MAX_LINE], read_buf[MAX_LINE];
    rudp_packet_t *rudp_pkt;
    FILE *file;
    cache_list_t cache;

    init_list(&cache);

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
//    sendto(sockfd, filename, strlen(filename), 0,
//           (struct sockaddr*)&serveraddr, sizeof(struct sockaddr_in) );
    u_int8_t seq_num = 0;
    rudp_pkt = create_rudp_packet(filename, strlen(filename), &seq_num);
    send_and_wait(sockfd, (struct sockaddr *)&serveraddr, rudp_pkt,
                  strlen(filename) + RUDP_HEAD);
    free(rudp_pkt);

    /*Open file write file*/
    sprintf(filename, "%s.out", filename);
    file = fopen(filename, "w+");

    /*Read file from server*/
    count = 0;
    cycle = 1;
    len = sizeof(struct sockaddr_in);
    u_int8_t last_packet= 0xFF;
    u_int16_t checksum;
    srand(time(NULL));
//    do{
    while(TRUE) {
//        int err = 0;
//        sleep(5);
        /*Receive packet from server*/
        memset(read_buf, 0, MAX_LINE);
        bytes_read = recvfrom(sockfd, read_buf, MAX_LINE,
                              0, (struct sockaddr *) &serveraddr,
                              (socklen_t *) &len);
        rudp_pkt = (rudp_packet_t *) read_buf;

        checksum = calc_checksum(rudp_pkt);

        /*Random packet error*/
        if(rand() % 10 == 0){
            checksum++;
        }

        /*Print packet contents to stdout*/
        fprintf(stdout, "\nGot %d byte packet\n", (int) bytes_read);
//        fprintf(stdout, "\t|-Sequence number: %d\n", rudp_pkt->seq_num);
//        fprintf(stdout, "\t|-Checksum: 0x%04x ", rudp_pkt->checksum);
//        fprintf(stdout, "(%s)\n", (checksum == 0) ? "correct" : "incorrect");
        bool good_checksum = print_rudp_packet(rudp_pkt);
        if(good_checksum && rudp_pkt->type == END_SEQ) {
            send_rudp_ack(sockfd, (struct sockaddr *) &serveraddr, rudp_pkt->seq_num);
            break;
        }
        count += bytes_read - RUDP_HEAD;

//        if( rudp_pkt->seq_num == (u_int8_t)(last_packet + 1) && checksum == 0){
//            /*Random packet drop*/
//            if(rand() % 10 == 0){
//                fprintf(stdout, "\t|-Packet dropped \n");
//            }
//            else {
//                fprintf(stdout, "\t|-Sending ACK for packet #%d\n", rudp_pkt->seq_num);
//
//                /*Write file to disk*/
//                fwrite(rudp_pkt->data, 1, bytes_read - RUDP_HEAD, file);
//
//                /*Send acknowledgement*/
//                send_rudp_ack(sockfd, (struct sockaddr *) &serveraddr, rudp_pkt->seq_num);
//                last_packet++;
//            }
//        }
        /*If the checksum is valid*/
        if (checksum == 0) {
            if(rand() % 5 == 0){
                fprintf(stdout, "\t|-Packet dropped (packet loss)\n");
                continue;
            }

            /*Send an acknowledgement*/
            fprintf(stdout, "\t|-Sending ACK for packet #%d\n", rudp_pkt->seq_num);
            send_rudp_ack(sockfd, (struct sockaddr *) &serveraddr, rudp_pkt->seq_num);

            /*Adjust file pointer to correct location for packet*/
            fseek(file, RUDP_DATA * cycle * rudp_pkt->seq_num, SEEK_SET);

            /*Write to file*/
            fprintf(stderr, "\t|-Writing packet %d to file\n", rudp_pkt->seq_num);
            fwrite(rudp_pkt->data, 1, (size_t) (bytes_read - RUDP_HEAD), file);
//            last_packet++;

            if (rudp_pkt->seq_num == 0xFF) {
                cycle++;
            }

            /*Check if packet received is anticipated next packet. If not, cache
             * this packet then try to find expected packet in the cache
             * (out of order delivery)*/
//            u_int8_t next_packet = (u_int8_t)(last_packet + 1);
//            if( rudp_pkt->seq_num != next_packet ){
//                node_t * to_cache = init_node(rudp_pkt, (size_t)bytes_read);
//                push_back(&cache, to_cache);
//                printf("\t|-Caching...\n");
//                write_cache(&cache, &last_packet, file);
//                perror("5");
//                node_t * retrieved = remove_node(&cache, next_packet );
//                if(retrieved != NULL){
////                    rudp_pkt = retrieved->data->rudp_pkt;
//                    bytes_read = retrieved->data->size;
//                    memcpy(rudp_pkt, retrieved->data->rudp_pkt, bytes_read);
//                    free_node(retrieved);
//                    fprintf(stdout, "Packet %d retrieved from cache\n", rudp_pkt->seq_num);
//                }
//                else{
//                    continue;
//                }
//            }
            /*If execution reached this point, the expected next packet was either just
             * received or successfully retrieved from the cache*/
//            else {
//                fprintf(stderr, "Writing packet %d to file\n", rudp_pkt->seq_num);
//                fwrite(rudp_pkt->data, 1, (size_t) (bytes_read - RUDP_HEAD), file);
//                last_packet++;
//            }
        } else {
            fprintf(stdout, "\t|-Packet dropped (bad checksum)\n");
        }
    }
//    }while( !(rudp_pkt->type == END_SEQ && rudp_pkt->seq_num == last_packet) );
    fprintf(stdout, "%d total bytes received\n", count);

    /*Clean up*/
    fclose(file);
    close(sockfd);
    return 0;
}

void write_cache(cache_list_t * cache, u_int8_t * prev_pkt, FILE * file){
    node_t * retrieved;
    u_int8_t next_packet = (u_int8_t)((*prev_pkt) + 1);

//    printf("Attempting to find packet %d in the cache...\n", (*prev_pkt)+1));
    while( (retrieved = remove_node(cache, next_packet)) != NULL ){
        fprintf(stdout, "\nPacket %d retrieved from cache\n",
                retrieved->data->rudp_pkt->seq_num);
//        perror("1");
        fprintf(stderr, "Writing packet %d to file\n",
                retrieved->data->rudp_pkt->seq_num);
        fwrite(retrieved->data->rudp_pkt, 1,
               retrieved->data->size - RUDP_HEAD, file);
//        perror("2");
        free_node(retrieved);
//        perror("3");
        (*prev_pkt)++;
        next_packet++;
//        perror("4");
    }
}