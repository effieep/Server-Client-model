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


#define perror2(s,e) fprintf(stderr, "%s: %s\n", s, strerror(e));

char** Create_Array_of_args(char*);
void issueJob(int,char* );
void setConcurrency(int ,char* );
void Stop_Job(char* );
void Poll(char*);
void Exit_Call();
void switch_command(int, char*);
