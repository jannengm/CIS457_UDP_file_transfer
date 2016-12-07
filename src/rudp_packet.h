/*******************************************************************************
 * CIS 457 - Project 4: Reliable File Transfer over UDP
 * rudp_packet.h header file
 * @author Mark Jannenga
 *
 * Defines constants and custom structs and declares functions used to create,
 * send, and manage Reliable UDP packets (RUDP packets).
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
    u_int32_t seq_num;              /*RUDP sequence number*/
    u_int8_t type;                  /*RUDP type*/
    u_int16_t checksum;             /*RUDP checksum*/
    unsigned char data[RUDP_DATA];  /*Binary data*/
};

/*Simple boolean enum*/
enum bool{
    FALSE, TRUE
};

/*Typedefs*/
typedef struct rudp_packet_t rudp_packet_t;
typedef enum bool bool;

/*******************************************************************************
 * Allocates memory for a new RUDP packet. Sets the data portion of the RUDP
 * packet to be equal top the first size bytes of the passed array of data.
 *
 * @param data - The binary data to be included in the RUDP packet
 * @param size - The size of the data parameter
 * @return pkt - A newly allocated pointer to an RUDP packet
 ******************************************************************************/
rudp_packet_t * create_rudp_packet(void *data, size_t size, u_int32_t *seq_num);

/*******************************************************************************
 * Computes the internet checksum for the entire RUDP packet. Assumes that the
 * checksum of the packet has been initialized to zero. If an RUDP packet with
 * and existing checksum is passed, this function returns 0 if the checksum is
 * correct.
 *
 * @param rudp_pk - The packet to calculate the checksum for
 * @return checksum - The internet checksum for the entire packet
 ******************************************************************************/
u_int16_t calc_checksum(rudp_packet_t * rudp_pk);

/*******************************************************************************
 * Checks if the checksum of rudp_pkt is correct. Returns TRUE if so, else FALSE
 *
 * @param rudp_pkt - The RUDP packet to check
 * @return TRUE or FALSE - Whether or not the checksum is correct
 ******************************************************************************/
bool check_checksum(rudp_packet_t * rudp_pkt);

/*******************************************************************************
 * Sends an acknowledgment for the packet designated by seq_num to the server.
 *
 * @param sockfd - The socket to send over
 * @param serveraddr - The address of the server
 * @param seq_num - The sequence number of the packet being acknowledged
 ******************************************************************************/
void send_rudp_ack(int sockfd, struct sockaddr *serveraddr,
                   rudp_packet_t * rudp_pkt);

/*******************************************************************************
 * Sends an RUDP packet (rudp_pkt) of a given size (size) to the destination
 * specified (destaddr) over the specified socket (sockfd). Waits a specified
 * amount of time (req) for an acknowledgement, then resends if no
 * acknowledgement was received. Attempts to send MAX_ATTEMPTS times, then
 * aborts if no acknowledgement was received. If an acknowledgement is received,
 * if is stored in the specified location (ack_pkt).
 *
 * @param sockfd - The soocket to send the message one
 * @param destaddr - The address of the destination to send to
 * @param rudp_pkt - The packet to send
 * @param size - The size of the packet to send
 * @param ack_pkt - The location to store the acknoowledgement
 * @param req - The amount of time to wait for an acknowledgement
 ******************************************************************************/
void send_and_wait(int sockfd, struct sockaddr *destaddr,
                   rudp_packet_t * rudp_pkt, size_t size,
                   rudp_packet_t * ack_pkt, struct timespec * req);

/*******************************************************************************
 * Prints data from the RUDP header to stdout. Checks the checksum and returns
 * TRUE if it is correct, else FALSE
 *
 * @param rudp_pkt - The packet to print
 * @return TRUE or FALSE - Whether or not the checksum is correct
 ******************************************************************************/
bool print_rudp_packet(rudp_packet_t * rudp_pkt);

#endif //PROJECT_4_UDP_PACKET_H
