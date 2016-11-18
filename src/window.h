//
// Created by jannengm on 11/18/16.
//

#ifndef PROJECT_4_WINDOW_H
#define PROJECT_4_WINDOW_H

#include "rudp_packet.h"

enum bool{
    FALSE, TRUE
};

struct window_t{
    struct rudp_packet_t *packets[WINDOW_SIZE];
    enum bool acknowledged[WINDOW_SIZE];
    int head;
    int tail;
};

typedef struct rudp_packet_t rudp_packet_t;
typedef struct window_t window_t;
typedef enum bool bool;

void init_window(window_t * window);
void fill_window(window_t * window, FILE * fd);
bool process_ack(window_t * window, rudp_packet_t * rudp_ack);

#endif //PROJECT_4_WINDOW_H
