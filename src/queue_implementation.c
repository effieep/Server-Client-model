#include "help_server.h"
#include "queue.h"

void Initialize_control_queue(control* ctrl,int bufsize){
    ctrl->front = NULL;
    ctrl->rear = NULL;
    ctrl->jobs_in_queue = 0;           //indicator of queue_position
    ctrl->total_jobs=0;                //indicator of id
    ctrl->max_jobs = bufsize;
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

void Initialize_Job(job_triplet* job,char* comm,control* ctrl,int cl_socket){
    assign_id(job,ctrl);
    int length = strlen(comm);
    job->command = malloc((length+1)*sizeof(char));
    strcpy(job->command,comm);
    job->queue_position = ctrl->jobs_in_queue;
    job->pid = 0;
    job->client_socket = cl_socket;
}

job_triplet* Enqueue(control *ctrl,char* command,int clientfd){
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
        Initialize_Job(new->job,command,ctrl,clientfd);
    }else{
        printf("Failed to allocate memory\n");
    }
    return new->job;
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
                Write_to_Commander(main_cursor->job->client_socket,"Job could not be executed because it was removed\n");
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


//Poll use
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
            strcat(output,"\n");
 
            main_cursor = main_cursor->next;
        } while (main_cursor != NULL);
    }else{
        strcpy(output,"Empty queue");
    }
    return output;
}

void free_job(job_triplet* job){
    free(job->command);
    free(job->job_id);
}

void Destroy_Queue(control* c){
    while(c->front != NULL) {
        struct node* tmp = c->front;
        c->front = c->front->next;
        free_job(tmp->job);
        free(tmp);
    }
    c->rear = NULL;
}

//For every job in the buffer, print the message "SERVER TERMINATED BEFORE EXECUTION"
void Inform_Clients(control* c){
    node* main_cursor;
    main_cursor = c->front;
    if(main_cursor != NULL){
        do
        {
            Write_to_Commander(main_cursor->job->client_socket,"SERVER TERMINATED BEFORE EXECUTION\n");
            main_cursor = main_cursor->next;
        } while (main_cursor != NULL);
    }
}