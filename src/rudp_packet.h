/*******************************************************************************
 *
 ******************************************************************************/

#ifndef PROJECT_4_UDP_PACKET_H
#define PROJECT_4_UDP_PACKET_H

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>

#define RUDP_HEAD 56        /*Size of RUDP header*/
#define RUDP_DATA 948       /*Size of RUDP data segment*/
#define MAX_LINE 1024       /*Maximum input buffer size*/
#define WINDOW_SIZE 5       /*Size of sliding window*/
#define MAX_ATTEMPTS 5      /*Maximum number of times to resend*/

/*RUDP types*/
#define DATA_PKT 0          /*Normal data packet*/
#define END_SEQ 1           /*Last packet in sequence*/
#define ACK 2               /*Acknowledgement*/
#define SYN 3               /*Initialize connection*/
#define SYN_ACK 4           /*Acknowledge open connection*/

/*Reliable UDP (RUDP) file transfer packet*/
struct rudp_packet_t{
    u_int32_t seq_num;               /*RUDP sequence number*/
    u_int8_t type;                  /*RUDP type*/
    u_int16_t checksum;             /*RUDP checksum*/
    unsigned char data[RUDP_DATA];  /*Binary data*/
};

enum bool{
    FALSE, TRUE
};

typedef struct rudp_packet_t rudp_packet_t;
typedef enum bool bool;
//typedef struct sockaddr sockaddr;

rudp_packet_t * create_rudp_packet(void * data, size_t size, u_int32_t * seq_num);
u_int16_t calc_checksum(rudp_packet_t * rudp_pk);
bool check_checksum(rudp_packet_t * rudp_pkt);
void send_rudp_ack(int sockfd, struct sockaddr *serveraddr, rudp_packet_t * rudp_pkt);
//void send_and_wait(int sockfd, struct sockaddr *destaddr, rudp_packet_t * rudp_pkt, size_t size);
//void send_and_wait(int sockfd, struct sockaddr *destaddr, rudp_packet_t * rudp_pkt,
//                   size_t size, rudp_packet_t * ack_pkt);
void send_and_wait(int sockfd, struct sockaddr *destaddr, rudp_packet_t * rudp_pkt,
                   size_t size, rudp_packet_t * ack_pkt, struct timespec * req);
bool print_rudp_packet(rudp_packet_t * rudp_pkt);


#endif //PROJECT_4_UDP_PACKET_H
