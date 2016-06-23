//
// Created by ths460 on 10/15/15.
//

#ifndef ASSIGNMENT4_SPACECATS_H
#define ASSIGNMENT4_SPACECATS_H
#define BUFSIZE 1024

struct __attribute((packed)) Data {
    int channels;
    int sample_rate;
    int sample_size;
    int error;

};

struct __attribute((packed)) audiofile {
    char buffer[BUFSIZE];
    int ID;
};
#endif //ASSIGNMENT4_SPACECATS_H
