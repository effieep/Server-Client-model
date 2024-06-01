#pragma once

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>	         /* toupper */
#include <signal.h>

char** Create_Array_of_args(char*);
void issueJob(char* command);
void setConcurrency(int sockfd,char* num);
void Stop_Job(char* job_ID);
void Poll(char* option);
void Exit_Call();
void switch_command(int sockfd, char* comm);