/*******************************************************************************
 *
 ******************************************************************************/

#ifndef PROJECT_4_UDP_PACKET_H
#define PROJECT_4_UDP_PACKET_H

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define RUDP_DATA 1000          /*Size of RUDP data segment*/
#define MAX_LINE 1024           /*Maximum input buffer size*/

/*Reliable UDP (RUDP) file transfer packet header*/
struct rudp_packet_t{
    u_int8_t seq_num;               /*RUDP sequence number*/
    u_int16_t checksum;             /*RUDP checksum*/
    unsigned char data[RUDP_DATA];
};

typedef struct rudp_packet_t rudp_packet_t;

rudp_packet_t * create_rudp_packet(void * data, size_t size);
u_int16_t calc_checksum(rudp_packet_t * rudp_pk);

#endif //PROJECT_4_UDP_PACKET_H
