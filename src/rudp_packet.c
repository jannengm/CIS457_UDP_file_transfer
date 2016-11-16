/*******************************************************************************
 *
 ******************************************************************************/

#include "rudp_packet.h"

rudp_packet_t * create_rudp_packet(void * data, size_t size){
    static u_int8_t seq;
    u_int16_t checksum;

    /*Allocate memory for the new RUDP packet*/
    rudp_packet_t * pkt = malloc(sizeof(rudp_packet_t));
    memset(pkt->data, 0, RUDP_DATA);
    pkt->checksum = 0;

    /*Initialize the packet with passed parameter*/
    pkt->seq_num = seq++;
    memcpy(pkt->data, data, size);

    /*Calculate RUDP checksum*/
    checksum = calc_checksum(pkt);
    pkt->checksum = checksum;

    return pkt;
}
u_int16_t calc_checksum(rudp_packet_t * rudp_pk){
    u_int16_t *data = (u_int16_t *)rudp_pk;
    int i;
    u_int32_t sum = 0;
    u_int16_t checksum;

    /*Loop throough the data 2 bytes at a time and add it up*/
    for(i = 0; i < sizeof(rudp_packet_t); i++){
        sum += data[i];
    }

    /*Initialize checksum to be the lower 16 bits of sum*/
    checksum = sum & 0xFFFF;

    /*Shift sum right 16 bits, leaving only the remainder (i.e. over flow)*/
    sum = sum >> 16;

    /*Add the remainder*/
    checksum += sum;

    /*Takes two's complement to get final checksum value*/
    checksum = ~checksum;

    return checksum;
}
