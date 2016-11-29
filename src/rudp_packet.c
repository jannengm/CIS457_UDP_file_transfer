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
rudp_packet_t * create_rudp_packet(void * data, size_t size, u_int32_t * seq_num){
    static u_int32_t seq;
    u_int16_t checksum;

    /*Allocate memory for the new RUDP packet*/
    rudp_packet_t * pkt = malloc(sizeof(rudp_packet_t));
    memset(pkt->data, 0, RUDP_DATA);

    /*Initialize the packet with passed parameter*/
    if(seq_num != NULL){
        pkt->seq_num = *seq_num;
    }
    else {
        pkt->seq_num = seq++;
    }
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
 * @return checksum - The internet checksum for the entire packet
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

bool check_checksum(rudp_packet_t * rudp_pkt){
    bool is_good = FALSE;

    u_int16_t tmp = rudp_pkt->checksum;
    rudp_pkt->checksum = 0;
    int checksum = calc_checksum(rudp_pkt);
    if(checksum == tmp){
        is_good = TRUE;
    }
    rudp_pkt->checksum = tmp;

    return is_good;
}

/*******************************************************************************
 * Sends an acknowledgment for the packet designated by seq_num to the server.
 *
 * @param sockfd - The socket to send over
 * @param serveraddr - The address of the server
 * @param seq_num - The sequence number of the packet being acknowledged
 ******************************************************************************/
void send_rudp_ack(int sockfd, struct sockaddr *serveraddr, rudp_packet_t * rudp_pkt){
    rudp_packet_t ack;
    memset(&ack, 0, sizeof(rudp_packet_t));
    ack.type = ACK;
    ack.checksum = 0;
    ack.seq_num = rudp_pkt->seq_num;
    ack.checksum = calc_checksum(&ack);

    sendto(sockfd, &ack, RUDP_HEAD, 0, serveraddr, sizeof(struct sockaddr_in));
}

void send_and_wait(int sockfd, struct sockaddr *destaddr, rudp_packet_t * rudp_pkt,
                   size_t size, rudp_packet_t * ack_pkt, struct timespec * req){
    struct pollfd fd;
    int err = 0, buf_len, timeout_ms, attempts = 0;
    unsigned char buffer[MAX_LINE];
    socklen_t len = sizeof(struct sockaddr_in);
    rudp_packet_t * ack;
    bool good_checksum;

    /*Convert timespec to milliseconds*/
    if(req != NULL) {
        timeout_ms = (int) (req->tv_sec * 1000 + req->tv_nsec / 1000000);
    }
    else{
        timeout_ms = 1000;      /*Default 1 second*/
    }

    fd.fd = sockfd;
    fd.events = POLLIN;

    u_int8_t expected = ACK;
    if(rudp_pkt->type == SYN){
        expected = SYN_ACK;
    }

    while(attempts < MAX_ATTEMPTS){
        /*Send packet to destination*/
        fprintf(stdout, "\nSending %d byte packet\n", (int) size);
        print_rudp_packet(rudp_pkt);
        sendto(sockfd, rudp_pkt, size, 0, destaddr, len);

        /*Wait up to 1000 ms for ACK*/
        err = poll(&fd, 1, timeout_ms);
        if(err == 0){
            fprintf(stdout,"\nTimeout, no ACK received\n");
        }
        else if(err < 0){
            fprintf(stderr, "\nPoll error\n");
            exit(1);
        }
        else{
            memset(buffer, 0, MAX_LINE);
            buf_len = (int)recvfrom(sockfd, buffer, MAX_LINE, 0, destaddr, &len);
            ack = (rudp_packet_t *)buffer;
//            fprintf(stdout, "\nGot %d byte packet\n", buf_len);
//            good_checksum = print_rudp_packet(ack);
            good_checksum = check_checksum(rudp_pkt);
            if(good_checksum &&
                    ack->seq_num == rudp_pkt->seq_num &&
                    ack->type == expected){
                fprintf(stdout, "\t|-RECEIVED ACKNOWLEDGEMENT\n");
                fprintf(stdout, "\nGot %d byte packet\n", buf_len);
                print_rudp_packet( (rudp_packet_t *)buffer );
                if(ack_pkt != NULL){
                    memset(ack_pkt, 0, sizeof(rudp_packet_t));
                    memcpy(ack_pkt, ack, (size_t) buf_len);
                }
                break;
            }
            else{
                fprintf(stdout, "\t|-RESENDING (%d)\n", attempts + 1);
            }
        }
        attempts++;
    }
    
    if(attempts >= MAX_ATTEMPTS){
        fprintf(stdout, "\t|-MAX ATTEMPTS REACHED, ABORTING\n");
    }
}


bool print_rudp_packet(rudp_packet_t * rudp_pkt){
//    u_int16_t orig_checksum = rudp_pkt->checksum;
//    rudp_pkt->checksum = 0;
//    int checksum = calc_checksum(rudp_pkt);
//    if(checksum == orig_checksum){
//        checksum = 0;
//    }
//    rudp_pkt->checksum = orig_checksum;
    bool checksum = check_checksum(rudp_pkt);

    fprintf(stdout, "\t|-TYPE:     0x%02x", rudp_pkt->type);
    switch(rudp_pkt->type){
        case DATA_PKT: fprintf(stdout, " (DATA_PKT)\n"); break;
        case END_SEQ: fprintf(stdout, " (END_SEQ)\n"); break;
        case ACK: fprintf(stdout, " (ACK)\n"); break;
        case SYN: fprintf(stdout, " (SYN)\n"); break;
        case SYN_ACK: fprintf(stdout, " (SYN_ACK)\n"); break;
        default: fprintf(stdout, " (UNKNOWN)\n"); break;
    }
    fprintf(stdout, "\t|-SEQ NUM:  %d\n", rudp_pkt->seq_num);
    fprintf(stdout, "\t|-CHECKSUM: 0x%04x ", rudp_pkt->checksum);
    fprintf(stdout, "(%s)\n", checksum ? "correct" : "incorrect");

    return checksum;
}