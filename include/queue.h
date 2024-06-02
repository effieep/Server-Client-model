#pragma once
#include "help_server.h"

typedef struct node {           //node of queue
    struct job_triplet* job;
    struct node *next;
}node;

typedef struct exec_node{           //node of queue
    struct job_triplet* job; 
    struct node *next;
}exec_node;

typedef struct control{           //for control
    int jobs_in_queue;
    int total_jobs;               //the total number of jobs that have been issued to jobexecutorserver
    int max_jobs;                 //bufferSize                 
    node *front;
    node *rear;
}control;

typedef struct job_triplet{         //info for job
    char* job_id;
    char* command;
    int queue_position;
    int client_socket;
    pid_t pid;
}job_triplet;


void Enqueue(control *,char*,int);
void Initialize_control_queue(control* ,int);
void Show_Queue(control* );
job_triplet* Dequeue(control* );
job_triplet* To_Be_Executed(control*);
void Remove_Pid(control* ,pid_t);
void Exec_Enqueue(control *,job_triplet*);
bool Remove_Job(control* ,char*);
char* Queue_Output(control* );
void Destroy_Queue(control*);