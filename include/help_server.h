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

#define BUFF_SIZE 1024
#define PERMS 0644	// access permission for the 3 groups of users

void swap_chars(char*,char* );
char* string_reverse(char *);
char* int_to_string(int );

void switch_command(int, char* );            //server
void Create_Indicator(pid_t);
void Read_from_Commander(int,char*);
void Write_to_Commander(int,char*);
void Manage_Jobs();
char** Create_Array_of_args(char*);
void Accept_Clients(int,char**);