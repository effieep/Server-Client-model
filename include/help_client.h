#pragma once

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>	     /* internet sockets */
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>
#include <stdbool.h>
#include <netdb.h>	         
#include <stdlib.h>
#include <errno.h>
#include <signal.h>

#define BUFF_SIZE 1024
#define PERMS 0644	// access permission for the 3 groups of users

void swap_chars(char* ,char*);
char* string_reverse(char *);
char* int_to_string(int );
void switch_command_C(char* );
void Write_to_Server(int, int, char**);
void Read_from_Server(int);
void Connect_to_Server(int,char**);