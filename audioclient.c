/* client.c
 *
 * part of the Systems Programming assignment
 * (c) Vrije Universiteit Amsterdam, 2005-2015. BSD License applies
 * author  : wdb -_at-_ few.vu.nl
 * contact : arno@cs.vu.nl
 * */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <dlfcn.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <errno.h>
#include <sys/time.h>

#include "library.h"
#include "audio.h"
#include "spacecats.h"

#define SERVER_PORT "9000"

static int breakloop = 0;    ///< use this variable to stop your wait-loop. Occasionally check its value, !1 signals that the program should close

/// unimportant: the signal handler. This function gets called when Ctrl^C is pressed
void sigint_handler(int sigint) {
    if (!breakloop) {
        breakloop = 1;
        printf("SIGINT catched. Please wait to let the client close gracefully.\nTo close hard press Ctrl^C again.\n");
    }
    else {
        printf("SIGINT occurred, exiting hard... please wait\n");
        exit(-1);
    }
}

int main(int argc, char *argv[]) {
    
    int server_fd, audio_fd;
    //client_filterfunc pfunc;
    //char buffer[BUFSIZE];
    
    
    struct addrinfo *result;
    //    long int numbytes, numbytesresv;
    //    struct timeval timeout;
    
    //    fd_set read_set;
    
    printf("handed in by Tisiana Henricus\n");
    
    signal(SIGINT, sigint_handler);    // trap Ctrl^C signals
    
    // parse arguments
    if (argc < 3) {
        printf("error : called with incorrect number of parameters\nusage : %s <server_name/IP> <filename> \n",
               argv[0]);
        return -1;
    }
    
    {
        struct addrinfo hints;
        int resolv;
        
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_protocol = 0;
        
        resolv = getaddrinfo(argv[1], SERVER_PORT, &hints, &result);
        if (resolv != 0) {
            fprintf(stderr, "getaddrinfo:%s\n", gai_strerror(resolv));
            return 1;
        }
    }
    
    server_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    
    if (server_fd < 0) {
        printf("error: unable to connect to server.\n");
        return -1;
    }
    
    {
        struct timeval timeout = {.tv_sec = 1, .tv_usec = 0};
        if (setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1){
            perror("Timeout could not be set");
            exit(-1);
        }
    }
    
    
    {
        //send audio filename to server
        int numbytes;
        
        numbytes = (int) sendto(server_fd, argv[2], strlen(argv[2]) + 1,
                                result->ai_flags,
                                result->ai_addr,
                                result->ai_addrlen);
        
        if (numbytes == -1) {
            perror("sendto");
            exit(1);
        }
    }
    
    
    {
        //receive data from audio file channels, sample size and sample rate
        int numbytesrecv;
        struct Data spacecats;
        
        numbytesrecv = (int) recvfrom(server_fd, &spacecats, sizeof(spacecats), result->ai_flags,
                                      result->ai_addr, &(result->ai_addrlen));
        if (numbytesrecv == -1) {
            perror("recvfrom");
            close(server_fd);
            return 1;
        }
        //checks if server returned an error
        if (spacecats.error == 0) {
            printf("audioserver failed to open audiofile, skipping request\n");
            close(server_fd);
            return 1;
        }
        
        printf("Channel %d, sample size %d, sample rate %d",
               spacecats.channels, spacecats.sample_size,
               spacecats.sample_rate);
        
        //open audio data information
        audio_fd = aud_writeinit(spacecats.sample_rate, spacecats.sample_size, spacecats.channels);
        
        if (audio_fd < 0) {
            printf("error: unable to open audio output.\n");
            return -1;
        }
    }
    
    {
        //stream audiofile and play
        struct audiofile wavfile;
        int numbytesrecv;
        
        int count;
        int bytessent;
        
        for (count = 1; ; count++) {
            
            
            
            //send a request for one packet
            bytessent = (int) sendto(server_fd, &count, sizeof(count), result->ai_flags,
                                     result->ai_addr, result->ai_addrlen);
            
            if (bytessent == -1) {
                perror("sendto");
                exit(1);
            }
            
            
            numbytesrecv = (int) recvfrom(server_fd, &wavfile, sizeof(struct audiofile),
                                          result->ai_flags, result->ai_addr, &(result->ai_addrlen));
            
            if (numbytesrecv == -1) {
                perror("recvfrom");
                close(server_fd);
                return 1;
            }
            printf("packet %d\n", wavfile.ID);
            
            
            if (count != wavfile.ID) {
                puts("SILENCIO");
                memset(wavfile.buffer, 0, BUFSIZE);
            }
            
            
            write(audio_fd, wavfile.buffer, BUFSIZE);
            
        }
        
    }
    
}





