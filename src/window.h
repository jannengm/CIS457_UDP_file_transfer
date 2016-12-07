/*******************************************************************************
 * CIS 457 - Project 4: Reliable File Transfer over UDP
 * window.h header file
 * @author Mark Jannenga
 *
 * Defines constants and custom structs and declares functions used to create,
 * send, and manage a sliding window in order to read in data from a file and
 * send it using RUDP packets.
 ******************************************************************************/

#ifndef PROJECT_4_WINDOW_H
#define PROJECT_4_WINDOW_H

#include "rudp_packet.h"

/*Custom struct to define a sliding window*/
struct window_t{
    struct rudp_packet_t *packets[WINDOW_SIZE]; //Array of pointers to packets
    int size[WINDOW_SIZE];                      //The size of each packet
    int head;                                   //First packet in window
    int tail;                                   //Next available spot in window
};

/*Typedefs*/
typedef struct window_t window_t;

/*******************************************************************************
 * Initializes an empty window (window)
 *
 * @param window - The window to initialize
 ******************************************************************************/
void init_window(window_t * window);

/*******************************************************************************
 * Inserts a single packet (rudp_pkt) of a specified size (size) into the window
 * (window). Returns TRUE if successful, else FALSE.
 *
 * @param window - The window to insert the packet into
 * @param rudp_pkt - The packet to insert
 * @param size - The size of the packet to insert
 * @return TRUE or FALSE - Whether or not insertion was successful
 ******************************************************************************/
bool insert_packet(window_t * window, rudp_packet_t * rudp_pkt, int size);

/*******************************************************************************
 * Fills the sliding window (window) with packets read in from a file (fd)
 *
 * @param window - The window to insert packets into
 * @param fd - The file to read data and create packets from
 ******************************************************************************/
void fill_window(window_t * window, FILE * fd);

/*******************************************************************************
 * Processes an RUDP acknowledgement packet (rudp_ack) and removes the
 * acknowledged packet from the sliding window (window) if it is present.
 * Returns TRUE if the acknowledged packet was successfully removed, else FALSE.
 *
 * @param window - The window too remove packets from
 * @param rudp_ack - The RUDP acknowledgement to process
 * @return TRUE or FALSE - whether or not the acknowledged packet was removed
 ******************************************************************************/
bool process_ack(window_t * window, rudp_packet_t * rudp_ack);

/*******************************************************************************
 * Advances the sliding window (window) as far as possible until an
 * unacknowledged packet is encountered.
 *
 * @param window - The sliding to to be advanced
 ******************************************************************************/
void advance_window(window_t * window);

/*******************************************************************************
 * Sends the entire window (window) of packets to a specified destination
 * (clientaddr) over a specified socket (sockfd). Prints data about each packet
 * as it is sent.
 *
 * @param window - The sliding window to be sent
 * @param sockfd - The socket to send the packets over
 * @param clientaddr - The destination to send the packets to
 ******************************************************************************/
void send_window(window_t * window, int sockfd, struct sockaddr* clientaddr);

/*******************************************************************************
 * Checks if the sliding window is empty or not. Returns TRUE if so, else FALSE.
 *
 * @param window - The sliding window to check
 * @return TRUE or FALSE - Whether or not the window is empty
 ******************************************************************************/
bool is_empty(window_t * window);

/*******************************************************************************
 * Prints the window to stderr.
 *
 * @param window - The window to print
 ******************************************************************************/
void print_window(window_t * window);

#endif //PROJECT_4_WINDOW_H
