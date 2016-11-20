//DEFUNCT

//
// Created by jannengm on 11/18/16.
//

#include "cache_list.h"

void init_list(cache_list_t * cache){
    cache->head = NULL;
    cache->tail = NULL;
}

node_t * init_node(rudp_packet_t * rudp_pkt, size_t size){
    node_t * n = malloc(sizeof(node_t));
    n->data = malloc(sizeof(cached_packet_t));
    n->data->rudp_pkt = malloc(size);
    n->data->size = size;
    memcpy(n->data->rudp_pkt, rudp_pkt, size);
    n->next = NULL;
    n->prev = NULL;
    return n;
}
void push_back(cache_list_t * cache, node_t * node){
    if(cache->tail == NULL && cache->head == NULL){
        cache->head = node;
        cache->tail = node;
    }
    else {
        cache->tail->next = node;
        node->prev = cache->tail;
        cache->tail = node;
    }
}
node_t * remove_node(cache_list_t * cache, int seq_num){
    node_t * tmp;
    for(tmp = cache->head; tmp != NULL; tmp = tmp->next){

        if(tmp->data->rudp_pkt->seq_num == seq_num) {
            /*Case with 1 element list*/
            if(tmp == cache->head && tmp == cache->tail){
                cache->head = NULL;
                cache->tail = NULL;
                break;
            }

            /*Case where node is at the head of the list*/
            if(tmp == cache->head){
                cache->head = cache->head->next;
                cache->head->prev = NULL;
                tmp->next = NULL;
                break;
            }

            /*Case where node is at the tail of the list*/
            else if(tmp == cache->tail){
                cache->tail = cache->tail->prev;
                cache->tail->next = NULL;
                tmp->prev = NULL;
                break;
            }

            /*Case where node is in the middle of the list*/
            else{
                tmp->prev->next = tmp->next;
                tmp->next->prev = tmp->prev;
                tmp->next = NULL;
                tmp->prev = NULL;
            }
        }
    }
    return tmp;
}
void free_node(node_t * node){
    free(node->data->rudp_pkt);
    free(node->data);
    free(node);
}
void free_list(cache_list_t * cache){
    node_t *to_free, *tmp = cache->head;

    while( tmp != NULL ){
        to_free = tmp;
        tmp = tmp->next;
        free_node(to_free);
    }
}