//
// Created by jannengm on 11/18/16.
//

#include "window.h"

void init_window(window_t * window){
    int i;
    for(i = 0; i < WINDOW_SIZE; i++){
        window->packets[i] = NULL;
        window->size[i] = 0;
    }
    window->head = 0;
    window->tail = 0;
}

bool insert_packet(window_t * window, rudp_packet_t * rudp_pkt, int size){
    if(window->tail < WINDOW_SIZE){
        window->packets[window->tail] = rudp_pkt;
        window->size[window->tail] = size;
        window->tail++;
        return TRUE;
    }
    return FALSE;
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
            rudp_pkt = create_rudp_packet(buffer, (size_t) buf_len, NULL);

            /*Add packet to window*/
            window->packets[window->tail] = rudp_pkt;
            window->size[window->tail] = buf_len + RUDP_HEAD;
            window->tail++;
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

bool is_empty(window_t * window){
    int i;
    for(i = 0; i < WINDOW_SIZE; i++){
        if(window->packets[i] != NULL)
            return FALSE;
    }
    return TRUE;
}

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
//    fprintf(stderr, "  HEAD: %d\t", window->head);
//    fprintf(stderr, "TAIL: %d\n", window->tail);
}