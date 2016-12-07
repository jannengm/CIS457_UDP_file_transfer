/*******************************************************************************
 *
 ******************************************************************************/

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "rudp_packet.h"
#include "window.h"
#include <pthread.h>

#define SEC_TO_NSEC 1000000000          /*Number of nanoseconds in 1 second*/
#define DEFAULT_TIMEOUT 100000000       /*Default timeout is 0.1 seconds*/

struct thread_arg_t{
    window_t * window;
    int sockfd;
};

typedef struct thread_arg_t thread_arg_t;

void send_file(int sockfd, struct sockaddr* clientaddr, FILE *file,
               struct timespec * req);
void * get_acks(void * arg);

/*Global semaphors for thread operations*/
pthread_mutex_t window_lock;
pthread_mutex_t flag_lock;
bool file_finished;

/*******************************************************************************
 * Server main method. Expects a port number as a command line argument
 *
 * @param argc
 * @param argv
 * @return
 ******************************************************************************/
int main(int argc, char **argv){
    int sockfd, len;
    ssize_t bytes_read;
    struct sockaddr_in serveraddr, clientaddr;
    char filename[MAX_LINE], buffer[MAX_LINE];
    FILE *file;
    rudp_packet_t *rudp_pkt;
    bool good_checksum, is_open;
    struct timespec req;

    /*Check command line arguments*/
    if(argc < 2 || argc > 3){
        fprintf(stderr, "Usage: %s [Port] [Timeout (optional)]\n", argv[0]);
        exit(1);
    }

    /*If timeout parameter specified, set timespec accordingly*/
    if(argc == 3){
        if( atof(argv[2]) >= 1 ){
            req.tv_sec = (int)(atof(argv[2]));
            req.tv_nsec = (int)((atof(argv[2]) - req.tv_sec) * SEC_TO_NSEC);
        }
        else{
            req.tv_sec = 0;
            req.tv_nsec = (int)(atof(argv[2]) * SEC_TO_NSEC);
        }
    }

    /*Otherwise, set default timeout to 0.1 seconds*/
    else{
        req.tv_sec = 0;
        req.tv_nsec = DEFAULT_TIMEOUT;
    }

    /*Create UDP socket*/
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0){
        printf("There was an error creating the socket\n");
        exit(1);
    }

    /*Set up server address and port*/
    serveraddr.sin_family=AF_INET;
    serveraddr.sin_port = htons( (uint16_t)atoi(argv[1]) );
    serveraddr.sin_addr.s_addr = INADDR_ANY;

    /*Bind server address and port to the socket*/
    bind(sockfd, (struct sockaddr*)&serveraddr, sizeof(struct sockaddr) );

    /*Listen for up to 10 clients*/
    listen(sockfd, 10);

    /*Wait for SYN*/
    len = sizeof(struct sockaddr_in);
    do {
        memset(buffer, 0, MAX_LINE);
        bytes_read = recvfrom(sockfd, buffer, MAX_LINE, 0,
                              (struct sockaddr *) &clientaddr,
                              (socklen_t *) &len);
        printf("Got %d byte packet\n", (int)bytes_read);
        good_checksum = print_rudp_packet( (rudp_packet_t*)buffer );
    }while( !good_checksum || ((rudp_packet_t *)buffer)->type != SYN);

    /*Attempt to open file*/
    memcpy(filename, ((rudp_packet_t*)buffer)->data,
           (size_t)(bytes_read - RUDP_HEAD));
    filename[bytes_read - RUDP_HEAD] = '\0';
    fprintf(stdout, "\nRequested file: %s\n", filename);

    file = fopen(filename, "r");
    if(file == NULL){
        fprintf(stderr, "Could not locate %s\n", filename);
        is_open = FALSE;
    }
    else {
        fprintf(stdout, "Successfully opened %s\n", filename);
        is_open = TRUE;
    }

    /*Create SYN_ACK packet*/
    u_int32_t seq_num = 0;
    rudp_pkt = create_rudp_packet(&is_open, sizeof(bool), &seq_num);
    rudp_pkt->type = SYN_ACK;
    rudp_pkt->checksum = 0;
    rudp_pkt->checksum = calc_checksum(rudp_pkt);

    /*Send SYN_ACK with status of file*/
    fprintf(stdout, "\nSending %d byte packet\n",
            (int)(sizeof(bool) + RUDP_HEAD) );
    print_rudp_packet(rudp_pkt);
    send_and_wait(sockfd, (struct sockaddr *) &clientaddr, rudp_pkt,
                  sizeof(bool) + RUDP_HEAD, NULL, &req);

    free(rudp_pkt);

    /*Read in file from disk*/
    if(is_open){
        send_file(sockfd, (struct sockaddr *) &clientaddr, file, &req);
    }

    close(sockfd);

    exit(0);
}

/*******************************************************************************
 *
 * @param sockfd
 * @param clientaddr
 * @param file
 ******************************************************************************/
void send_file(int sockfd, struct sockaddr* clientaddr, FILE *file,
               struct timespec * req){
    pthread_t child;
    thread_arg_t arg;
    window_t window;

    /*Initialize the sliding window*/
    init_window(&window);

    /*Set flag to indicate file not yet sent*/
    file_finished = FALSE;

    /*Detach thread to listen for ACKs*/
    arg.sockfd = sockfd;
    arg.window = &window;
    if( pthread_create(&child, NULL, get_acks, &arg) != 0) {
        printf("Failed to create thread\n");
        exit(1);
    }
    pthread_detach(child);

    while(TRUE){
        pthread_mutex_lock(&window_lock);

        /*Check exit conditions*/
        if(feof(file) && is_empty(&window)) {
            pthread_mutex_unlock(&window_lock);
            break;
        }

        /*Update window and send*/
        advance_window(&window);
        fill_window(&window, file);
        send_window(&window, sockfd, clientaddr);

        pthread_mutex_unlock(&window_lock);

        /*Wait for acknowledgements*/
        struct timespec rem;
        nanosleep(req, &rem);
    }

    /*Send END_SEQ packet*/
    rudp_packet_t end_seq;
    memset(&end_seq, 0, sizeof(rudp_packet_t));
    end_seq.type = END_SEQ;
    end_seq.checksum = calc_checksum(&end_seq);
    send_and_wait(sockfd, clientaddr, &end_seq, RUDP_HEAD, NULL, req);

    pthread_mutex_lock(&flag_lock);
    file_finished = TRUE;
    pthread_mutex_unlock(&flag_lock);

    /*Clean up*/
    fclose(file);
}

/*******************************************************************************
 *
 * @param arg
 * @return
 ******************************************************************************/
void * get_acks(void * arg){
    unsigned char buffer[MAX_LINE];
    struct sockaddr_in clientaddr;
    int buf_len, len;
    int sockfd = ((thread_arg_t *)arg)->sockfd;
    window_t * window = ((thread_arg_t *)arg)->window;

    while(TRUE) {

        /*If parent thread has finished sending the file, exit*/
        pthread_mutex_lock(&flag_lock);
        if(file_finished){
            pthread_mutex_unlock(&flag_lock);
            break;
        }
        else {
            pthread_mutex_unlock(&flag_lock);
        }

        /*If the window is empty, do not block to wait for ACKs*/
        pthread_mutex_lock(&window_lock);
        if( is_empty(window) ){
            pthread_mutex_unlock(&window_lock);
            continue;
        }
        pthread_mutex_unlock(&window_lock);

        /*Block and wait for ACKs*/
        buf_len = (int) recvfrom(sockfd, buffer, MAX_LINE, 0,
                                 (struct sockaddr *) &clientaddr,
                                 (socklen_t *) &len);

        /*If ACK received, try to remove it from the window*/
        if (((rudp_packet_t *) buffer)->type == ACK) {
            fprintf(stdout, "Received %d byte acknowledgement for packet %d\n",
                    buf_len, ((rudp_packet_t *) buffer)->seq_num);
            pthread_mutex_lock(&window_lock);
            process_ack(window, (rudp_packet_t *) buffer);
            pthread_mutex_unlock(&window_lock);
        }
    }

    return NULL;
}