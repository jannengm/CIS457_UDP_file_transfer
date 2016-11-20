//DEFUNCT

//
// Created by jannengm on 11/18/16.
//

#ifndef PROJECT_4_CACHE_LIST_H
#define PROJECT_4_CACHE_LIST_H

#include "rudp_packet.h"

struct cache_list_t{
    struct node_t * head;
    struct node_t * tail;
};

struct node_t{
    struct node_t * prev;
    struct node_t * next;
    struct cached_packet_t * data;
};

struct cached_packet_t{
    rudp_packet_t * rudp_pkt;
    size_t size;
};

typedef struct cache_list_t cache_list_t;
typedef struct node_t node_t;
typedef struct cached_packet_t cached_packet_t;

void init_list(cache_list_t * cache);
node_t * init_node(rudp_packet_t * rudp_pkt, size_t size);
void push_back(cache_list_t * cache, node_t * node);
node_t * remove_node(cache_list_t * cache, int seq_num);
void free_node(node_t * node);
void free_list(cache_list_t * cache);

#endif //PROJECT_4_CACHE_LIST_H
