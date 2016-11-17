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

#define RUDP_HEAD 24        /*Size of RUDP header*/
#define RUDP_DATA 980       /*Size of RUDP data segment*/
#define MAX_LINE 1024       /*Maximum input buffer size*/
#define WINDOW_SIZE 5       /*Size of sliding window*/

/*RUDP types*/
#define DATA_PKT 0          /*Normal data packet*/
#define END_SEQ 1           /*Last packet in sequence*/
#define ACK 2               /*Acknowledgement*/

/*Reliable UDP (RUDP) file transfer packet*/
struct rudp_packet_t{
    u_int8_t seq_num;               /*RUDP sequence number*/
    u_int8_t type;                  /*RUDP type*/
    u_int16_t checksum;             /*RUDP checksum*/
    unsigned char data[RUDP_DATA];  /*Binary data*/
};

struct window_t{
    struct rudp_packet_t *packets[WINDOW_SIZE];
    int head;
    int tail;
};

enum bool{
    FALSE, TRUE
};

typedef struct rudp_packet_t rudp_packet_t;
typedef struct window_t window_t;
typedef enum bool bool;
//typedef struct sockaddr sockaddr;

rudp_packet_t * create_rudp_packet(void * data, size_t size);
u_int16_t calc_checksum(rudp_packet_t * rudp_pk);
//bool is_full(rudp_packet_t * window[]);
//bool is_empty(rudp_packet_t * window[]);
//void fill_window(rudp_packet_t * window[], FILE *file);
//bool remove_rudp_packet(rudp_packet_t * window[], u_int8_t seq);
//void send_ack(int sockfd, struct sockaddr *serveraddr, u_int8_t seq_num);
void init_window(window_t * window);

#endif //PROJECT_4_UDP_PACKET_H
