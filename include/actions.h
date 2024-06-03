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

void Initialize_buffer(int);
void Cond_Initialization();
char** Create_Array_of_args(char*);
void issueJob(int,char* );
void setConcurrency(int ,char* );
void Stop_Job(int,char* );
void Poll(int);
void Exit_Call();
void switch_command(int, char*);
job_triplet* Read_Buffer();
void Exec_Job(job_triplet*);