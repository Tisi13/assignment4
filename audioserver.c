/* server.c
 *
 * part of the Systems Programming assignment
 * (c) Vrije Universiteit Amsterdam, 2005-2015. BSD License applies
 * author  : wdb -_at-_ few.vu.nl
 * contact : arno@cs.vu.nl
 * */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <errno.h>


#include "library.h"
#include "audio.h"
#include "spacecats.h"

/// a define used for the copy buffer in stream_data(...)
//#define BUFSIZE 1024
#define MYPORT 9000
#define MAX_FILE_LEN 1000

static int breakloop = 0;    ///< use this variable to stop your wait-loop. Occasionally check its value, !1 signals that the program should close


int stream_data(int client_fd, struct sockaddr *addr, socklen_t addrlen, char *file_name) {
    int data_fd;
    //server_filterfunc pfunc;
    // char *datafile, *libfile;



    // open input
    {
        int numbytessent;
        //char msg[MAX_FILE_LEN];
        struct Data sample;

        //read in audio file and retrieve channels, sample size and sample rate
        data_fd = aud_readinit(file_name, &sample.sample_rate, &sample.sample_size, &sample.channels);

        if (data_fd < 0) {
            printf("failed to open datafile %s, skipping request\n", file_name);
            sample.error = 0;

            // sends client an error message
            numbytessent = (int) sendto(client_fd, &sample, sizeof(struct Data), 0, addr, addrlen);

            if (numbytessent == -1) {
                perror("sendto");
            }
            return -1;
        }

        //sends client data from audio file
        numbytessent = (int) sendto(client_fd, &sample, sizeof(struct Data), 0, addr, addrlen);

        if (numbytessent == -1) {
            perror("sendto");
            return -1;
        }

        sample.error = 1;
        printf("opened datafile %s, channel %d, samplesize %d, samplerate %d\n", file_name, sample.channels,
               sample.sample_size, sample.sample_rate);


        // sends audio file data to client
        {
            struct timeval timeout = {.tv_sec = 1, .tv_usec = 0};
            if (setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1){
                perror("Timeout could not be set");
                return 1;
            }
        }


        {
            int bytesread, numbytesresv;
            struct audiofile wavfile;
            int something = 1;
           //


            for (wavfile.ID = 1; ; wavfile.ID++) {

                //receives request for one packet
                
                numbytesresv = (int) recvfrom(client_fd, &something, sizeof(something), 0, addr, &addrlen);

                if (numbytesresv <= -1) {
                    perror("recvfrom id");

                    //
                    return 1;
                }

                printf("something: %d\n", something);


                bytesread = (int) read(data_fd, wavfile.buffer, BUFSIZE);

                if (bytesread == -1) {
                    perror("read");
                    close(data_fd);
                    return -1;
                }

                if (bytesread == 0) {
                    close(data_fd);
                    breakloop;
                    return -1;

                }


                numbytessent = (int) sendto(client_fd, &wavfile, sizeof(struct audiofile), 0, addr, addrlen);
                printf("sent data packet %d\n", wavfile.ID);

                if (numbytessent == -1) {
                    perror("sendto");
                    return -1;

                }

                if (wavfile.ID % 10 == 0) {
                    usleep(40000);
                }

                //close(data_fd);

            }


        }

    }


}

/// unimportant: the signal handler. This function gets called when Ctrl^C is pressed
void sigint_handler(int sigint) {
    if (!breakloop) {
        breakloop = 1;
        printf("SIGINT catched. Please wait to let the server close gracefully.\nTo close hard press Ctrl^C again.\n");
    }
    else {
        printf("SIGINT occurred, exiting hard... please wait\n");
        exit(-1);
    }
}

/// the main loop, continuously waiting for clients
int main(void) {

    int client_fd;
    struct sockaddr_in6 addr;
    socklen_t addr_len = sizeof(struct sockaddr_in6);


    printf("SysProg network server\n");
    printf("handed in by Tisiana Henricus\n");

    signal(SIGINT, sigint_handler);    // trap Ctrl^C signals

    client_fd = socket(AF_INET6, SOCK_DGRAM, 0);

    if (client_fd <= -1) {
        perror("socket");
        return 1;
    }


    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(MYPORT);

    if (bind(client_fd, (struct sockaddr *) &addr, addr_len) <= -1) {
        perror("bind");
        close(client_fd);
        return 1;
    }

    while (!breakloop) {

        int numbytes;
        char file_name[MAX_FILE_LEN];
        struct timeval timeout = {.tv_sec = 0, .tv_usec= 0};

        // receives audio filename from client
        numbytes = (int) recvfrom(client_fd, file_name, MAX_FILE_LEN, 0, (struct sockaddr *) &addr, &addr_len);

        if (numbytes <= -1) {
            perror("recvfrom");
            close(client_fd);
            return 1;
        }

        puts(file_name);

        //start streaming data to client
        stream_data(client_fd, (struct sockaddr *) &addr, addr_len, file_name);

        setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        sleep(1);
    }

    return 0;
}

