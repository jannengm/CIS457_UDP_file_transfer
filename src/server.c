/*******************************************************************************
 *
 ******************************************************************************/

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "rudp_packet.h"
#include "window.h"
#include <pthread.h>

struct thread_arg_t{
    window_t * window;
    int sockfd;
};

typedef struct thread_arg_t thread_arg_t;

void send_file(int sockfd, struct sockaddr* clientaddr, FILE *file);
void * get_acks(void * arg);

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
//    unsigned char read_buf[RUDP_DATA];
    FILE *file;
//    rudp_packet_t *rudp_pkt;

    /*Check command line arguments*/
    if(argc != 2){
        fprintf(stderr, "Usage: %s [Port]\n", argv[0]);
        exit(1);
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

    /*Get file name from client*/
    len = sizeof(struct sockaddr_in);
    memset(buffer, 0, MAX_LINE);
    bytes_read = recvfrom(sockfd, buffer, MAX_LINE, 0,
                          (struct sockaddr*)&clientaddr,
                          (socklen_t *)&len);
    printf("Got %d byte packet\n", (int)bytes_read);
    print_rudp_packet( (rudp_packet_t*)buffer );

    printf("\t|-SENDING ACKNOWLEDGEMENT\n");
    send_rudp_ack(sockfd, (struct sockaddr*)&clientaddr, ((rudp_packet_t*)buffer)->seq_num );

    memcpy(filename, ((rudp_packet_t*)buffer)->data, bytes_read - RUDP_HEAD);
    filename[bytes_read - RUDP_HEAD] = '\0';
    fprintf(stdout, "\nRequested file: %s\n", filename);

    /*Attempt to open the file*/
    file = fopen(filename, "r");
    if(file == NULL){
        fprintf(stderr, "Could not locate %s\n", filename);
        close(sockfd);
        exit(1);
    }
    fprintf(stdout, "Successfully opened %s\n", filename);

    /*Read in file from disk*/
    send_file(sockfd, (struct sockaddr *)&clientaddr, file);

    close(sockfd);

    exit(0);
}

/*******************************************************************************
 *
 * @param sockfd
 * @param clientaddr
 * @param file
 ******************************************************************************/
void send_file(int sockfd, struct sockaddr* clientaddr, FILE *file){
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
//        sleep(1);
        struct timespec req, rem;
        req.tv_sec = 0;
        req.tv_nsec = 100000000;
        nanosleep(&req, &rem);
    }

    /*Send END_SEQ packet*/
    rudp_packet_t end_seq;
    memset(&end_seq, 0, sizeof(rudp_packet_t));
    end_seq.type = END_SEQ;
    end_seq.checksum = calc_checksum(&end_seq);
    send_and_wait(sockfd, clientaddr, &end_seq, RUDP_HEAD);

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