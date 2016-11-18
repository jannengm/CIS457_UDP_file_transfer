//
// Created by jannengm on 11/18/16.
//

#include "window.h"

void init_window(window_t * window){
    int i;
    for(i = 0; i < WINDOW_SIZE; i++){
        window->packets[i] = NULL;
        window->acknowledged[i] = FALSE;
    }
    window->head = -1;
    window->tail = 0;
}

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
            rudp_pkt = create_rudp_packet(buffer, (size_t) buf_len);
            if(feof(fd)){
                rudp_pkt->type = END_SEQ;
                rudp_pkt->checksum = 0;
                rudp_pkt->checksum = calc_checksum(rudp_pkt);
            }

            /*Add packet to window*/
            window->packets[window->tail] = rudp_pkt;
            window->acknowledged[window->tail] = FALSE;
            window->tail++;

            /*Update head if window was empty*/
            if(window->head < 0){
                window->head++;
            }
        }
    }
}

/*******************************************************************************
 *
 * @param window
 * @param rudp_ack
 * @return
 ******************************************************************************/
bool process_ack(window_t * window, rudp_packet_t * rudp_ack){
    int i, j;

    /*Loop through all packets in the window*/
    for(i = 0; i < WINDOW_SIZE; i++){
        /*If the packet is found in the window, remove it*/
        if(window->packets[i]->seq_num == rudp_ack->seq_num){
            window->acknowledged[i] = TRUE;
            free(window->packets[i]);
            window->packets[i] = NULL;

            /*If this packet is at the head of the window, advance head*/
            if(i == window->head){
                for(j = window->head; j < WINDOW_SIZE; j++){
                    if(window->packets[i] == NULL)
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

void advance_window(window_t * window){
    int i;

    for(i = 0; i + window->head < WINDOW_SIZE; i++){
        window->packets[i] = window->packets[window->head + i];
        window->acknowledged[i] = window->acknowledged[window->head + i];

        window->packets[window->head + i] = NULL;
        window->acknowledged[window->head + i] = FALSE;
    }

    window->tail -= window->head;
    window->head = 0;
}