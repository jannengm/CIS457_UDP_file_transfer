/*******************************************************************************
 *
 ******************************************************************************/

#ifndef PROJECT_4_UDP_PACKET_H
#define PROJECT_4_UDP_PACKET_H

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#define RUDP_HEAD 24        /*Size of RUDP header*/
#define RUDP_DATA 980       /*Size of RUDP data segment*/
#define MAX_LINE 1024       /*Maximum input buffer size*/
#define MAX_SEQ 0xFE        /*Maximum sequence number*/
#define END_SEQ 0xFF        /*End of sequence flag*/
#define WINDOW_SIZE 5       /*Size of sliding window*/

/*Reliable UDP (RUDP) file transfer packet*/
struct rudp_packet_t{
    u_int8_t seq_num;               /*RUDP sequence number*/
    u_int16_t checksum;             /*RUDP checksum*/
    unsigned char data[RUDP_DATA];  /*Binary data*/
};

enum bool{
    FALSE, TRUE
};

typedef struct rudp_packet_t rudp_packet_t;
typedef enum bool bool;

rudp_packet_t * create_rudp_packet(void * data, size_t size);
u_int16_t calc_checksum(rudp_packet_t * rudp_pk);
bool is_full(rudp_packet_t * window[]);
bool is_empty(rudp_packet_t * window[]);
void fill_window(rudp_packet_t * window[], FILE *file);
bool remove_rudp_packet(rudp_packet_t * window[], u_int8_t seq);

#endif //PROJECT_4_UDP_PACKET_H
