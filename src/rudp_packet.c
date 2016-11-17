/*******************************************************************************
 *
 ******************************************************************************/

#include "rudp_packet.h"

/*******************************************************************************
 * Allocates memory for a new RUDP packet. Sets the data portion of the RUDP
 * packet to be equal top the first size bytes of the passed array of data.
 *
 * @param data - The binary data to be included in the RUDP packet
 * @param size - The size of the data parameter
 * @return pkt - A newly allocated pointer to an RUDP packet
 ******************************************************************************/
rudp_packet_t * create_rudp_packet(void * data, size_t size){
    static u_int8_t seq;
    u_int16_t checksum;

    /*Allocate memory for the new RUDP packet*/
    rudp_packet_t * pkt = malloc(sizeof(rudp_packet_t));
    memset(pkt->data, 0, RUDP_DATA);

    /*Initialize the packet with passed parameter*/
    pkt->seq_num = seq++;
    pkt->checksum = 0;
    pkt->type = DATA_PKT;
    memcpy(pkt->data, data, size);

    /*Calculate RUDP checksum*/
    checksum = calc_checksum(pkt);
    pkt->checksum = checksum;

    return pkt;
}

/*******************************************************************************
 * Computes the internet checksum for the entire RUDP packet. Assumes that the
 * checksum of the packet has been initialized to zero. If an RUDP packet with
 * and existing checksum is passed, this function returns 0 if the checksum is
 * correct.
 *
 * @param rudp_pk - The packet to calculate the checksum for
 * @return checksum - The internet checksum computer for the entire packet
 ******************************************************************************/
u_int16_t calc_checksum(rudp_packet_t * rudp_pk){
    u_int16_t *data = (u_int16_t *)rudp_pk;
    int i;
    u_int32_t sum = 0;
    u_int16_t checksum;

    /*Loop through the data 2 bytes at a time and add it up*/
    for(i = 0; i < sizeof(rudp_packet_t) / 2; i++){
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

///*******************************************************************************
// * Returns a boolean value designating whether or not the window is full
// *
// * @param window - Pointer to the first element in the window
// * @return TRUE or FALSE
// ******************************************************************************/
//bool is_full(rudp_packet_t * window[]){
//    return (bool) (window[WINDOW_SIZE - 1] == NULL);
//}
//
///*******************************************************************************
// * Returns a boolean value designating whether or not the window is empty
// *
// * @param window - Pointer to the first element in the window
// * @return TRUE or FALSE
// ******************************************************************************/
//bool is_empty(rudp_packet_t * window[]){
//    int i;
//    for(i = 0; i < WINDOW_SIZE; i++){
//        if(window[i] != NULL){
//            return FALSE;
//        }
//    }
//    return TRUE;
//}
//
///*******************************************************************************
// * Reads in up RUDP_DATA bytes from file at a time, creates a new RUDP packet,
// * and adds it to the first available slot in the window
// *
// * @param window
// * @param file
// ******************************************************************************/
//void fill_window(rudp_packet_t * window[], FILE *file){
//    int i;
//    unsigned char buffer[MAX_LINE];
//    ssize_t bytes_read;
//    rudp_packet_t *rudp_pkt;
//
//    for(i = 0; i < WINDOW_SIZE && window[i] == NULL; i++){
//        memset(buffer, 0, MAX_LINE);
//        /*Read up to RUDP_DATA bytes from file*/
//        bytes_read = fread(buffer, 1, RUDP_DATA, file);
//        if(ferror(file)){
//            fprintf(stderr, "File read error\n");
//            break;
//        }
//
//        /*If bytes successfully read in, send RUDP packet*/
//        if(bytes_read > 0) {
//            /*Create a new RUDP packet*/
//            rudp_pkt = create_rudp_packet(buffer, (size_t) bytes_read);
//
//            /*If file reached EOF, flag as the last packet*/
//            if (feof(file)) {
//                rudp_pkt->seq_num = END_SEQ;
//                rudp_pkt->checksum = 0;
//                rudp_pkt->checksum = calc_checksum(rudp_pkt);
//            }
//
//            /*Insert new RUDP packet into window*/
//            window[i] = rudp_pkt;
//        }
//    }
//}
//
///*******************************************************************************
// * Searches through the window of RUDP packets for the specificied sequence
// * number. Deallocates memory for the packet, sets the window slot to be NULL,
// * and returns TRUE if the packet is found, else returns FALSE.
// *
// * @param window - The window of RUDP packets
// * @param seq - The sequence number of the packet to remove
// * @return Whether or not the packet was removed
// ******************************************************************************/
//bool remove_rudp_packet(rudp_packet_t * window[], u_int8_t seq){
//    int i;
//    for(i = 0; i < WINDOW_SIZE; i++){
//        if(window[i] == NULL)
//            continue;
//        if(window[i]->seq_num == seq){
//            free(window[i]);
//            window[i] = NULL;
//            return TRUE;
//        }
//    }
//    return FALSE;
//}
//
///*******************************************************************************
// * Sends an acknowledgment for the packet designated by seq_num to the server.
// *
// * @param sockfd - The socket to send over
// * @param serveraddr - The address of the server
// * @param seq_num - The sequence number of the packet being acknowledged
// ******************************************************************************/
//void send_ack(int sockfd, struct sockaddr *serveraddr, u_int8_t seq_num){
//    rudp_packet_t ack;
//    ack.type = ACK;
//    ack.checksum = 0;
//    ack.seq_num = seq_num;
//    ack.checksum = calc_checksum(&ack);
//
//    sendto(sockfd, &ack, RUDP_HEAD, 0, serveraddr, sizeof(struct sockaddr_in));
//}

void init_window(window_t * window){
    int i;
    for(i = 0; i < WINDOW_SIZE; i++){
        window->packets[i] = NULL;
    }
    window->head = -1;
    window->tail = -1;
}