#include <stdbool.h>

#include "queue.h"
#include "help_server.h"

void Initialize_control_queue(control* ctrl){
    ctrl->front = NULL;
    ctrl->rear = NULL;
    ctrl->jobs_in_queue = 0;                //indicator of queue_position
    ctrl->total_jobs=0;                     //indicator of id
}

void assign_id(job_triplet* job,control* ctrl){
    char n[] = "job_";
    char *id = malloc(2*sizeof(char));
    int num = ctrl->total_jobs;
    id = int_to_string(num);
    job->job_id = malloc(6*sizeof(char));
    strcpy(job->job_id,n);
    strcat(job->job_id,id);
}

void Initialize_Job(job_triplet* job,char* comm,control* ctrl){
    assign_id(job,ctrl);
    int length = strlen(comm);
    job->command = malloc((length+1)*sizeof(char));
    strcpy(job->command,comm);
    job->queue_position = ctrl->jobs_in_queue;
    job->pid = 0;
}

void Exec_Enqueue(control *ctrl,job_triplet* job){
    node* new = malloc(sizeof(node));
    if(new != NULL){
        new->job = malloc(sizeof(job_triplet));
        new->next = NULL;
        if(ctrl->front== NULL){         //if list is empty
            ctrl->front = new;
            ctrl->rear = new;
        }
        else{                           //If list is not empty
            ctrl->rear->next = new;     //add the new node to the end of the list
            ctrl->rear = new;
        }
        ctrl->jobs_in_queue++;
        ctrl->total_jobs++;
        new->job = job;
        //fix queue position
        new->job->queue_position = ctrl->jobs_in_queue;
    }else{
        printf("Failed to allocate memory\n");
    }
}

void Enqueue(control *ctrl,char* command){
    node* new = malloc(sizeof(node));
    if(new != NULL){
        new->job = malloc(sizeof(job_triplet));
        new->next = NULL;

        if(ctrl->front== NULL){                                             //if list is empty
            ctrl->front = new;
            ctrl->rear = new;
        }
        else{                                                               //If list is not empty
            ctrl->rear->next = new;                                         //add the new node to the end of the list
            ctrl->rear = new;
        }
        ctrl->jobs_in_queue++;
        ctrl->total_jobs++;
        Initialize_Job(new->job,command,ctrl);
    }else{
        printf("Failed to allocate memory\n");
    }
}

void fix_queue_position(control* ctrl){
    node* main_cursor;
    main_cursor = ctrl->front->next;
    if(main_cursor != NULL){
        do
        {
            main_cursor->job->queue_position--;
            main_cursor = main_cursor->next;
        } while (main_cursor != NULL);
    }
}

job_triplet* Dequeue(control* ctrl){
    //node to be removed from the front part of queue
    if(ctrl->jobs_in_queue == 0){                   //queue is empty                                     
        printf("Empty queue\n");
        return NULL;  
    }
    node* new_front;
    job_triplet* removed = ctrl->front->job;
    if(ctrl->front->next != NULL){                  //more than one node
        new_front = ctrl->front->next;
        ctrl->jobs_in_queue--;
        fix_queue_position(ctrl);      
    }else{                                          //only one node in the queue
        new_front = NULL;
        ctrl->rear = NULL;
        ctrl->jobs_in_queue = 0;   
    }
    free(ctrl->front);
    ctrl->front = new_front;
    return removed;
}

void Show_Queue(control* c){
    node* main_cursor;
    main_cursor = c->front;
    //printf("The queue is : \n");
    if(main_cursor != NULL){
        do
        {
            printf("%s,%d,%d  ->  ", main_cursor->job->job_id,main_cursor->job->queue_position,main_cursor->job->pid);
            main_cursor = main_cursor->next;
        } while (main_cursor != NULL);
    }
    printf("\n");
}

void Remove_node(control*c,node* prev,node* current){
    if(c->front == current){                //first node in the queue           
        c->front = current->next;
    }else if(c->rear == current){           //last node in the queue
        prev->next = NULL;
        c->rear = prev; 
    }else if(c->jobs_in_queue == 1){        //queue has only one job
        c->front = NULL;
        c->rear = NULL;
    }else{                          
        prev->next = current->next;         //node in the middle of the queue
    }
    if(current->job->pid != 0){             //Terminate the current process
        kill(current->job->pid,SIGKILL);
    }
    c->jobs_in_queue--;
    node* main_cursor = current->next;
    //fix the queue position of the next nodes
    if(main_cursor != NULL){
        do
        {
            main_cursor->job->queue_position--;
            main_cursor = main_cursor->next;
        } while (main_cursor != NULL);
    }
    free(current->job);
    free(current);
}

//Termination of a child process use
void Remove_Pid(control* c,pid_t pid){
    node* main_cursor;
    node* prev;
    main_cursor = c->front;
    prev = NULL;
    if(main_cursor != NULL){
        do  
        {   //check if the pid is the same
            if(main_cursor->job->pid == pid){
                Remove_node(c,prev,main_cursor);
                break;
            }
            prev = main_cursor;
            main_cursor = main_cursor->next;
        } while (main_cursor != NULL);
    }
    printf("\n");
}

//Stop use
bool Remove_Job(control* c,char* id){
    node* main_cursor;
    node* prev;
    bool found = false;
    main_cursor = c->front;
    prev = NULL;
    if(main_cursor != NULL){
        do
        {   //Check if the job_id is th same
            if(strcmp(main_cursor->job->job_id,id) == 0){
                Remove_node(c,prev,main_cursor);
                found = true;
                break;
            }
            prev = main_cursor;
            main_cursor = main_cursor->next;
        } while (main_cursor != NULL);
    }
    return found;
}


//Poll use [runnning,queued]
char* Queue_Output(control* c){
    node* main_cursor;
    char* output = malloc(BUFF_SIZE*sizeof(char));
    strcpy(output,"");
    main_cursor = c->front;
    //create the message consisted of all the jobs of the queue
    if(main_cursor != NULL){
        do
        {   
            strcat(output,main_cursor->job->job_id);
            strcat(output," ");
            strcat(output,main_cursor->job->command);
            strcat(output," ");
            char* pos = malloc(2*sizeof(char));
            pos = int_to_string(main_cursor->job->queue_position);
            strcat(output,pos);
            strcat(output,"\n");
            free(pos);
            main_cursor = main_cursor->next;
        } while (main_cursor != NULL);
    }else{
        strcpy(output,"Empty queue");
    }
    return output;
}