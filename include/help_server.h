#pragma once

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>	         /* toupper */
#include <signal.h>
#include <netdb.h>
#include <netinet/in.h>	     /* internet sockets */
#include <pthread.h>
#include "queue.h"

#define BUFF_SIZE 1024
#define PERMS 0644	// access permission for the 3 groups of users

void swap_chars(char*,char* );
char* string_reverse(char *);
char* int_to_string(int );        

char* Read_from_Commander(int);
void Write_to_Commander(int,char*);

void Manage_Jobs();
void *Controller_Thread(void*);
void Accept_Clients(char**);

job_triplet* Place_to_Buffer(int ,char*);
void lock();
void unlock();
void lockbuffer();
void unlockbuffer();
void broadcast_fill();
void broadcast_concurr();
void Inform_Worker_Threads(int );
void Disable_restart();

typedef struct ctrl_args{
    int socket;             //client socket
    pthread_t* wth;         //pointer in the array of worker threads id
    int threads;
}ctrl_args;