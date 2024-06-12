#include "help_server.h"
#include "queue.h"
#include "actions.h"

static int concurrencyLevel = 1;
static control buffer;

void issueJob(int cl_socket,char* command){
    job_triplet* job_rem = Place_to_Buffer(cl_socket,command);
    //Send the job triplet back to commander
    char *buff = malloc(100*sizeof(char));
    strcpy(buff,"");           //reinitialize buffer
    strcat(buff,"JOB ");
    strcat(buff,job_rem->job_id);
    strcat(buff," ");
    strcat(buff,job_rem->command);
    strcat(buff," ");
    strcat(buff,"SUBMITTED\n");
    Write_to_Commander(cl_socket,buff);
    free(buff);
}

void setConcurrency(int sockfd,char* num){
    lock();                                         //lock emtx
    concurrencyLevel = atoi(num);
    Inform_Worker_Threads(concurrencyLevel);        //Enable worker threads if needed
    unlock();                                       //unlock emtx
    char *response = malloc(21 *sizeof(char));
    strcpy(response,"CONCURRENCY SET AT ");
    strcat(response,num);
    strcat(response,"\n");
    Write_to_Commander(sockfd,response);
    free(response);
}

void Stop_Job(int sockfd,char* job_ID){
    char* id = malloc(6*sizeof(char));
    int fd;
    char *buff = malloc(10*sizeof(char));
    char *message = malloc(20*sizeof(char));
    strcpy(buff,"");                            //reinitialize buffer
    strcpy(id,job_ID);
    bool found = Remove_Job(&buffer,job_ID);
    if(found){
        //Create message JOB <jobid> REMOVED
        strcpy(message,"JOB ");
        strcat(buff,id);
        strcat(buff," ");
        strcat(message,buff);   
        strcat(message,"REMOVED\n");
    }else{
        //Create message JOB <jobid> NOT FOUND
        strcpy(message,"JOB ");
        strcat(buff,id);
        strcat(buff," ");
        strcat(message,buff);   
        strcat(message,"NOT FOUND\n");
    }
    Write_to_Commander(sockfd,message);
    free(buff);
    free(message);
    free(id);
}

void Poll(int sockfd){
    char* buff = malloc(BUFF_SIZE*sizeof(char));
    char* out = Queue_Output(&buffer);
    strcpy(buff,out);
    Write_to_Commander(sockfd,buff);
    free(out);
    free(buff);
}

void Exit_Call(int sockfd,pthread_t* wth,int threads){
    int fd,err;
    char *response = malloc(20*sizeof(char));
    strcpy(response,"");
    char mess[] = "SERVER TERMINATED\n";
    strcat(response,mess);
    Disable_restart();
    //Unblock the worker threads blocked to a condition
    broadcast_concurr();
    broadcast_fill();
    //Wait for every worker thread to terminate
    for(int i=0;i<threads;i++){
        if((err = pthread_join(wth[i], NULL)) < 0){
            perror2("pthread_join",err);
        }
    }
    //Send message "SERVER TERMINATED BEFORE EXECUTION" to the 
    //clients that their job is in the buffer
    Inform_Clients(&buffer);
    Destroy_Queue(&buffer);
    Write_to_Commander(sockfd,response);
    free(response);
    exit(0);
}

void switch_command(int sockfd, char* comm,pthread_t* wth,int threads){
    const char delim[] = " ";
    char* temp = malloc(BUFF_SIZE*sizeof(char));
    strcpy(temp,comm);
    char *tok = strtok(temp, delim);
    //issueJob case
    if(strcmp(tok,"issueJob")== 0){
        char* result = malloc(BUFF_SIZE * sizeof(char));
        strcpy(result,"");
        //Isolate issueJob from the rest of string
        //keep only the unix command in the variable 
        char *token = strtok(comm, delim);
        while (token != NULL) {
            token = strtok(NULL, delim); 
            if(token != NULL){
                strcat(result,token);
                strcat(result," ");
            }
        }
        result[strlen(result) - 1] = '\0';
        issueJob(sockfd,result);
        free(result);
    //setConcurrency case
    }else if(strcmp(tok,"setConcurrency")==0){
        char *num = strtok(NULL, delim); 
        setConcurrency(sockfd,num);
    //stop case
    }else if(strcmp(tok,"stop")==0){
        char *job = strtok(NULL, delim); 
        Stop_Job(sockfd,job);
    //poll case
    }else if(strcmp(tok,"poll")==0){
        Poll(sockfd);
    //exit case
    }else if(strcmp(tok,"exit")==0){
        Exit_Call(sockfd,wth,threads);
    }else{
        printf("Command does not exists.Try again!\n");
    }
    free(temp);
}

//create pid.out file of the child process - job output
int Create_File(pid_t pid,char* jobid){
    int fd;
    char* id = malloc(15*sizeof(char));
    id = int_to_string(pid);
    char* filename = malloc(25*sizeof(char));
    strcpy(filename,id);
    strcat(filename,".out");
    if ( (fd = open(filename,  O_CREAT | O_RDWR | O_APPEND, PERMS)) == -1){
        perror("creating");
        exit(1);
    }
    free(filename);
    free(id);
    return fd;
}

void Return_job_output(job_triplet* job,int pid){
    int fd;
    //Specify the filename of pid.out
    char* id = malloc(10*sizeof(char));
    id = int_to_string(pid);
    strcat(id,".out");

    if ( (fd = open(id, O_RDWR | O_APPEND, PERMS)) == -1){
        perror("creating");
        exit(1);
    }

    //Calculate the size of file
    struct stat file_status;
    if (stat(id, &file_status) < 0) {
        perror("stat");
        exit(1);
    }
    int fsize = file_status.st_size;

    //Allocate appropriate memory
    char* output = malloc((fsize+80)*sizeof(char));

    //Create first line of the output
    char *startline = malloc(30*sizeof(char));
    strcpy(startline,"\0");
    strcpy(startline,"-----");
    strcat(startline,job->job_id);
    strcat(startline," output start-----\n\n");

    //Add first line
    strcat(output,startline);

    //read BUFF_SIZE chars of the file and add them to the final output 
    char* file_output = malloc(BUFF_SIZE*sizeof(char));
    int bytes_read;
    while ((bytes_read = read(fd, file_output, BUFF_SIZE)) > 0) {
        strcat(output,file_output);
        memset(file_output,'\0',BUFF_SIZE*sizeof(char));
    }
    // Create the last line of the output
    char *endline = malloc(30*sizeof(char));
    strcpy(endline,"\n-----");
    strcat(endline,job->job_id);
    strcat(endline," output end-----");
    strcat(output,endline);

    //Respond to the Client
    Write_to_Commander(job->client_socket,output);
    memset(output,'\0',(fsize+80)*sizeof(char));

    //Delete the file created for the specific process
    if(unlink(id) < 0){
        perror("unlink");
    }
    free(id);
    free(file_output);
    free(output);
    free(endline);
    free(startline);
}

void Exec_Job(job_triplet* exec_job){
    pid_t pid;
    int fd;
    char **args;
    int status,count;
    char* fn;
    int stdoutCopy = dup(1); 
    if(exec_job != NULL){
        args = Create_Array_of_args(exec_job->command,&count);
        pid = fork();
        if(pid == -1){
            perror("fork");
            exit(1);
        }else if(pid > 0){              //parent process
            if (waitpid(pid,&status,0) == -1) {
                // wait failed
                perror("wait");
                exit(1);
            }
        }else {                         //child process
            fd = Create_File(getpid(),exec_job->job_id);
            dup2(fd,1);
            if(execvp(args[0],args)==-1){
                perror("execv");
            }
        }
    }
    Return_job_output(exec_job,pid);
    for(int i=0;i<count;i++){
        free(args[i]);
    }
    free(args);
    dup2(stdoutCopy,1);
    close(stdoutCopy);
}

char** Create_Array_of_args(char* command,int* c){
    char* temp1 = malloc((strlen(command)+1)*sizeof(char));
    char* temp2 = malloc((strlen(command)+1)*sizeof(char));
    strcpy(temp1,command);
    strcpy(temp2,command);
    const char delim[] = " ";
    char **args;
    char *token = strtok(temp1, delim);
    int count;
    *c = count;
    // Iterate through the tokens
    while (token != NULL) {
        count++;                        // number of arguments
        token = strtok(NULL, delim);    // Get the next token
    }
    args = malloc(count * sizeof(char*));

    // Tokenize the string
    token = strtok(temp2, delim);
    args[0] = malloc((strlen(token)+1)*sizeof(char));
    strcpy(args[0],token);
    count = 1;
    while (token != NULL) {
        token = strtok(NULL, delim); // Get the next token
        if(token != NULL){
            args[count] = malloc((strlen(token)+1)*sizeof(char));
            strcpy(args[count],token);
            count++;
        }
    }
    args[count] = NULL;
    free(temp1);
    free(temp2);
    return args;
}

void Initialize_buffer(int bufferSize){
    Initialize_control_queue(&buffer,bufferSize);
}

control* get_buffer(){
    return &buffer;
}

int getconcurrency(){
    return concurrencyLevel;
}