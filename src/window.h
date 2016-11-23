//
// Created by jannengm on 11/18/16.
//

#ifndef PROJECT_4_WINDOW_H
#define PROJECT_4_WINDOW_H

#include "rudp_packet.h"

struct window_t{
    struct rudp_packet_t *packets[WINDOW_SIZE];
    int size[WINDOW_SIZE];
    int head;
    int tail;
};

typedef struct rudp_packet_t rudp_packet_t;
typedef struct window_t window_t;

void init_window(window_t * window);
bool insert_packet(window_t * window, rudp_packet_t * rudp_pkt, int size);
void fill_window(window_t * window, FILE * fd);
bool process_ack(window_t * window, rudp_packet_t * rudp_ack);
void advance_window(window_t * window);
void send_window(window_t * window, int sockfd, struct sockaddr* clientaddr);
bool is_empty(window_t * window);
void print_window(window_t * window);

#endif //PROJECT_4_WINDOW_H
