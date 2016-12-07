/*******************************************************************************
 * CIS 457 - Project 4: Reliable File Transfer over UDP
 * window.c source code
 * @author Mark Jannenga
 *
 * Implements functions delcared in window.h
 ******************************************************************************/

#include "window.h"

/*******************************************************************************
 * Initializes an empty window (window)
 *
 * @param window - The window to initialize
 ******************************************************************************/
void init_window(window_t * window){
    int i;
    for(i = 0; i < WINDOW_SIZE; i++){
        window->packets[i] = NULL;
        window->size[i] = 0;
    }
    window->head = 0;
    window->tail = 0;
}

/*******************************************************************************
 * Inserts a single packet (rudp_pkt) of a specified size (size) into the window
 * (window). Returns TRUE if successful, else FALSE.
 *
 * @param window - The window to insert the packet into
 * @param rudp_pkt - The packet to insert
 * @param size - The size of the packet to insert
 * @return TRUE or FALSE - Whether or not insertion was successful
 ******************************************************************************/
bool insert_packet(window_t * window, rudp_packet_t * rudp_pkt, int size){
    if(window->tail < WINDOW_SIZE){
        window->packets[window->tail] = rudp_pkt;
        window->size[window->tail] = size;
        window->tail++;
        return TRUE;
    }
    return FALSE;
}

/*******************************************************************************
 * Fills the sliding window (window) with packets read in from a file (fd)
 *
 * @param window - The window to insert packets into
 * @param fd - The file to read data and create packets from
 ******************************************************************************/
void fill_window(window_t * window, FILE * fd){
    unsigned char buffer[MAX_LINE];
    rudp_packet_t *rudp_pkt;
    int buf_len;

    while( window->tail < WINDOW_SIZE && !feof(fd) ){
        buf_len = (int) fread(buffer, 1, RUDP_DATA, fd);

        if( ferror(fd) ){
            fprintf(stderr, "File read error\n");
            fclose(fd);
            exit(1);
        }

        /*If read from file was successful*/
        if(buf_len > 0){

            /*Create new RUDP packet*/
            rudp_pkt = create_rudp_packet(buffer, (size_t) buf_len, NULL);

            /*Add packet to window*/
            window->packets[window->tail] = rudp_pkt;
            window->size[window->tail] = buf_len + RUDP_HEAD;
            window->tail++;
        }
    }
}

/*******************************************************************************
 * Processes an RUDP acknowledgement packet (rudp_ack) and removes the
 * acknowledged packet from the sliding window (window) if it is present.
 * Returns TRUE if the acknowledged packet was successfully removed, else FALSE.
 *
 * @param window - The window too remove packets from
 * @param rudp_ack - The RUDP acknowledgement to process
 * @return TRUE or FALSE - whether or not the acknowledged packet was removed
 ******************************************************************************/
bool process_ack(window_t * window, rudp_packet_t * rudp_ack){
    int i, j;

    /*Loop through all packets in the window*/
    for(i = 0; i < WINDOW_SIZE; i++){
        if(window->packets[i] == NULL)
            continue;

        /*If the packet is found in the window, remove it*/
        if(window->packets[i]->seq_num == rudp_ack->seq_num){
            free(window->packets[i]);
            window->packets[i] = NULL;

            /*If this packet is at the head of the window, advance head*/
            if(i == window->head){
                for(j = window->head; j < WINDOW_SIZE; j++){
                    if(window->packets[j] == NULL)
                        window->head++;
                    else
                        break;
                }
            }

            /*Packet found*/
            return TRUE;
        }
    }

    /*Made it through the window without finding the packet*/
    return FALSE;
}

/*******************************************************************************
 * Advances the sliding window (window) as far as possible until an
 * unacknowledged packet is encountered.
 *
 * @param window - The sliding to to be advanced
 ******************************************************************************/
void advance_window(window_t * window){
    int i;

    for(i = 0; i + window->head < WINDOW_SIZE && window->head != 0; i++){
        window->packets[i] = window->packets[window->head + i];
        window->size[i] = window->size[window->head + i];

        window->packets[window->head + i] = NULL;
        window->size[window->head + i] = 0;
    }

    window->tail -= window->head;
    window->head = 0;
}

/*******************************************************************************
 * Sends the entire window (window) of packets to a specified destination
 * (clientaddr) over a specified socket (sockfd). Prints data about each packet
 * as it is sent.
 *
 * @param window - The sliding window to be sent
 * @param sockfd - The socket to send the packets over
 * @param clientaddr - The destination to send the packets to
 ******************************************************************************/
void send_window(window_t * window, int sockfd, struct sockaddr* clientaddr){
    static int bytes_sent;
    int i;
    bool good_checksum;
    u_int16_t checksum;

    print_window(window);
    for(i = 0; i < WINDOW_SIZE; i++){
        if(window->packets[i] != NULL){
            fprintf(stdout, "\nSending %d byte packet\n", window->size[i]);
            good_checksum = print_rudp_packet(window->packets[i]);
            if(!good_checksum){
                fprintf(stdout, "\t\t|-CHECKSUM CALC RESULT: 0x%04x\n",
                        calc_checksum(window->packets[i]));
                fprintf(stdout, "\t|-RECALCULATING CHECKSUM\n");
                window->packets[i]->checksum = 0;
                checksum = calc_checksum(window->packets[i]);
                window->packets[i]->checksum = checksum;
            }
            sendto(sockfd, window->packets[i], (size_t) window->size[i], 0,
                   clientaddr, sizeof(struct sockaddr));
            bytes_sent += window->size[i] - RUDP_HEAD;
        }
    }
    fprintf(stdout, "%d total bytes sent\n", bytes_sent);
}

/*******************************************************************************
 * Checks if the sliding window is empty or not. Returns TRUE if so, else FALSE.
 *
 * @param window - The sliding window to check
 * @return TRUE or FALSE - Whether or not the window is empty
 ******************************************************************************/
bool is_empty(window_t * window){
    int i;
    for(i = 0; i < WINDOW_SIZE; i++){
        if(window->packets[i] != NULL)
            return FALSE;
    }
    return TRUE;
}

/*******************************************************************************
 * Prints the window to stderr.
 *
 * @param window - The window to print
 ******************************************************************************/
void print_window(window_t * window){
    int i;
    fprintf(stderr, "\n|");
    for(i = 0; i < WINDOW_SIZE; i++){
        if(window->packets[i] != NULL){
            fprintf(stderr, " %3d ", window->packets[i]->seq_num);
        }
        else{
            fprintf(stderr, "     ");
        }
    }
    fprintf(stderr, "|\n");
    fprintf(stderr, "---------------------------\n");
}