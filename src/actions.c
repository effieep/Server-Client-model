#include "help_server.h"
#include "queue.h"
#include "actions.h"

static int concurrency = 1;
static control c_waiting,c_executing;
char *workfile="jobExecutorServer.txt";


void handle_enable_server(int signo){
    printf("Server activated!\n");
}  

void child_handler(int signo){
    int status;
    pid_t pid;
    printf("Signal that child terminated!\n");

    pid = waitpid(-1, &status, WNOHANG);
    if (pid > 0) {
        if (WIFEXITED(status)) {
            printf("Child process %d terminated normally with exit status: %d\n", pid, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Child process %d terminated by signal: %d\n", pid, WTERMSIG(status));
        }
    } else if (pid == -1) {
        perror("waitpid");
    }
}

void issueJob(char* command){
    Enqueue(&c_waiting,command);
    job_triplet* job_rem = c_waiting.rear->job;

    //Send the job triplet back to commander
    char *buff = malloc(100*sizeof(char));
    strcpy(buff,"");            //reinitialize buffer
    //printf("content of buffer %s\n",buff);
    strcat(buff,job_rem->job_id);
    strcat(buff," ");
    strcat(buff,job_rem->command);
    strcat(buff," ");
    char* pos = malloc(2*sizeof(char));
    pos = int_to_string(job_rem->queue_position);
    strcat(buff,pos);
    free(pos);
    int fd1;
    sleep(1);
    //Write_to_Commander("issue",buff);
    free(buff);
}

void setConcurrency(int sockfd,char* num){
    concurrency = atoi(num);
    char *response = malloc(21 *sizeof(char));
    strcpy(response,"CONCURRENCY SET AT ");
    strcat(response,num);
    printf("Response is %s\n",response);
    Write_to_Commander(sockfd,response);
    printf("Concurrency now is: %d\n",concurrency);
}

void Stop_Job(char* job_ID){
    char* id = malloc(6*sizeof(char));
    int fd;
    char *buff = malloc(20*sizeof(char));
    char *message = malloc(10*sizeof(char));
    strcpy(buff,"");                            //reinitialize buffer
    strcpy(id,job_ID);
    //printf("jobid is: %s\n",id);
    bool found_in_wait = Remove_Job(&c_waiting,job_ID);
    //printf("found in wait? %d\n",found_in_wait);
    if(found_in_wait){
        //Create message job_XX removed
       strcpy(message,"removed");
       strcat(buff,id);
       strcat(buff," ");
       strcat(buff,message);   
    }else{
        bool found_in_exec = Remove_Job(&c_executing,job_ID);
        if(found_in_exec){
            //Create message job_XX terminated
            strcpy(message,"terminated");
            strcat(buff,id);
            strcat(buff," ");
            strcat(buff,message);   
        }else{
            strcat(buff,"Job does not exist\n");
        }
    }
    sleep(1);
    //Write_to_Commander("stop",buff);
    free(buff);
    free(message);
    free(id);
}

void Poll(char* option){
    char* buff = malloc(BUFF_SIZE*sizeof(char));
    if(strcmp(option,"queued") == 0){
        char* out = Queue_Output(&c_waiting);
        strcpy(buff,out);
    }else if(strcmp(option,"running") == 0){
        char* out = Queue_Output(&c_executing);
        strcpy(buff,out);
    }
    sleep(1);
    //Write_to_Commander("poll",buff);
    free(buff);
}

void Exit_Call(){
    int fd;
    char buff[] = "jobExecutorServer terminated";
    if(unlink(workfile) == -1){
        perror("unlink");
        exit(-1);
    }
    if(unlink("myfifo") == -1){
        perror("unlink");
        exit(-1);
    }
    sleep(1);
    //Write_to_Commander("exit",buff);
    //Destroy queue
    exit(0);
}

void switch_command(int sockfd, char* comm){
    const char delim[] = " ";
    char* temp = malloc(BUFF_SIZE*sizeof(char));
    strcpy(temp,comm);
    char *tok = strtok(temp, delim);
    if(strcmp(tok,"issueJob")== 0){
        printf("issuejob case\n");
        //issueJob(comm);
    }else if(strcmp(tok,"setConcurrency")==0){
        printf("setConcurrency case\n");
        char *num = strtok(NULL, delim); 
        setConcurrency(sockfd,num);
    }else if(strcmp(tok,"stop")==0){
        char *job = strtok(NULL, delim); 
        //Stop_Job(job);
    }else if(strcmp(tok,"poll")==0){
        char *option = strtok(NULL, delim); 
        //Poll(option);
    }else if(strcmp(tok,"exit")==0){
        Exit_Call();
    }else{
        printf("Command does not exists.Try again!\n");
    }
    free(temp);
}

void Exec_Jobs(){
    job_triplet *exec_job = c_executing.front->job;
    node* cursor = c_executing.front;
    pid_t pid;
    //Execute jobs in executing queue
    do{
        exec_job = cursor->job;
        cursor = cursor->next;
        if(exec_job != NULL){
            if(exec_job->pid == 0){
                char **args = Create_Array_of_args(exec_job->command);
                pid = fork();
                if(pid == -1){
                    perror("fork");
                }else if(pid > 0){              //parent process
                    exec_job->pid = pid;
                }else {                         //child process
                    if(execvp(args[0],args)==-1){
                        perror("execv");
                    }
                }
            }
        }
    }while(c_executing.jobs_in_queue > 0 && cursor != NULL);  //inifinite loop giati den meiono to jobs_in_queue

}

void Fill_Exec_Queue(){
    job_triplet* curr_job;
    int jobs;

    jobs = concurrency - c_executing.jobs_in_queue;
    if(jobs>0){         //more jobs can be added in the executing queue
        for(int i=1;i<=jobs;i++){
            if(c_waiting.jobs_in_queue > 0){
                curr_job = Dequeue(&c_waiting);
                Exec_Enqueue(&c_executing,curr_job);    
            }
        }
    }else{              //some jobs must be removed from the executing queue
        jobs = c_executing.jobs_in_queue - concurrency;
        for(int i=1;i<=jobs;i++){
            if(c_executing.front != NULL){
                job_triplet* removed = c_executing.front->job;
                bool found = Remove_Job(&c_executing,removed->job_id);
                if(found){
                    printf("Job removed\n");
                }
            }else{
                break;
            }
        }
    }
}

void Manage_Jobs(){
    job_triplet *exec_job;
    int keep_going = 1;
    //SIGCHLD handler
    static struct sigaction act;
    act.sa_handler= child_handler;
    sigaction(SIGCHLD, &act, NULL);

    Fill_Exec_Queue();
    Exec_Jobs();
}

char** Create_Array_of_args(char* command){
    char temp1[BUFF_SIZE],temp2[BUFF_SIZE];
    strcpy(temp1,command);
    strcpy(temp2,command);
    const char delim[] = " ";
    char **args;
    char *token = strtok(temp1, delim);
    int count = 0;
    // Iterate through the tokens
    while (token != NULL) {
        count++;
        token = strtok(NULL, delim); // Get the next token
    }
    args = malloc(count * sizeof(char*));

    // Tokenize the string
    token = strtok(temp2, delim);
    //skip first token which contains issueJob
    count = 0;
    while (token != NULL) {
        token = strtok(NULL, delim); // Get the next token
        if(token != NULL){
            args[count] = malloc((strlen(token)+1)*sizeof(char));
            strcpy(args[count],token);
            count++;
        }
    }
    args[count] = NULL;
    return args;
}
