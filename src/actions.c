#include "help_server.h"
#include "queue.h"
#include "actions.h"

static int concurrencyLevel = 1;
static control buffer,c_executing;

void issueJob(int cl_socket,char* command){
    printf("In issueJob sockfd is : %d\n",cl_socket);
    Place_to_Buffer(cl_socket,command);
    job_triplet* job_rem = buffer.rear->job;

    //Send the job triplet back to commander
    char *buff = malloc(100*sizeof(char));
    strcpy(buff,"");            //reinitialize buffer
    //printf("content of buffer %s\n",buff);
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
    lock();
    concurrencyLevel = atoi(num);
    Inform_Worker_Threads(concurrencyLevel);
    unlock();
    char *response = malloc(21 *sizeof(char));
    strcpy(response,"CONCURRENCY SET AT ");
    strcat(response,num);
    strcat(response,"\n");
    printf("Response is %s\n",response);
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
    //printf("jobid is: %s\n",id);
    bool found = Remove_Job(&buffer,job_ID);
    //printf("found in wait? %d\n",found_in_wait);
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
    free(buff);
}

void Exit_Call(int sockfd){
    int fd;
    char *response = malloc(50*sizeof(char));
    strcpy(response,"");
    char mess[] = "SERVER TERMINATED\n";
    strcat(response,mess);
    bool client_found = Search_Client(&buffer,sockfd);
    if(client_found){
        char buff[] = "SERVER TERMINATED BEFORE EXECUTION\n";
        strcat(response,buff);
    }
    Destroy_Queue(&buffer);
    Write_to_Commander(sockfd,response);
    free(response);
    exit(0);
}

void switch_command(int sockfd, char* comm){
    const char delim[] = " ";
    char* temp = malloc(BUFF_SIZE*sizeof(char));
    strcpy(temp,comm);
    char *tok = strtok(temp, delim);
    if(strcmp(tok,"issueJob")== 0){
        printf("issuejob case\n");
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
    }else if(strcmp(tok,"setConcurrency")==0){
        printf("setConcurrency case\n");
        char *num = strtok(NULL, delim); 
        setConcurrency(sockfd,num);
    }else if(strcmp(tok,"stop")==0){
        char *job = strtok(NULL, delim); 
        Stop_Job(sockfd,job);
    }else if(strcmp(tok,"poll")==0){
        Poll(sockfd);
    }else if(strcmp(tok,"exit")==0){
        Exit_Call(sockfd);
    }else{
        printf("Command does not exists.Try again!\n");
    }
    free(temp);
}

int Create_File(pid_t pid,char* jobid){
    int fd;
    printf("fd = %d\n",fd);
    char* id = malloc(10*sizeof(char));
    id = int_to_string(pid);
    char* filename = malloc(15*sizeof(char));
    strcpy(filename,id);
    strcat(filename,".out");
    printf("filename is %s\n",filename);
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
        exit(EXIT_FAILURE);
    }
    int fsize = file_status.st_size;
    printf("size of the file is: %d\n",fsize);

    //Allocate appropriate memory
    char* output = malloc((fsize+80)*sizeof(char));

    //Create first line of the output
    char *startline = malloc(30*sizeof(char));
    strcpy(startline,"-----");
    strcat(startline,job->job_id);
    strcat(startline," output start-----\n\n");

    //Add first line
    strcat(output,startline);

    //check how many chars th file contains and allocate them 
    char* file_output = malloc(BUFF_SIZE*sizeof(char));
    int bytes_read;
    while ((bytes_read = read(fd, file_output, BUFF_SIZE)) > 0) {
        printf("Read %d bytes from the output file\n",bytes_read);
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
    // free(id);
    // free(file_output);
    // free(output);
    // free(endline);
    // free(startline);
}

void Exec_Job(job_triplet* exec_job){
    pid_t pid;
    int fd;
    int status;
    char* fn;
    int stdoutCopy = dup(1); 
    if(exec_job != NULL){
        char **args = Create_Array_of_args(exec_job->command);
        pid = fork();
        if(pid == -1){
            perror("fork");
        }else if(pid > 0){              //parent process
            if (waitpid(pid,&status,0) == -1) {
                // wait failed
                perror("wait");
                exit(EXIT_FAILURE);
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
    dup2(stdoutCopy,1);
    close(stdoutCopy);
}

void Fill_Exec_Queue(){
    job_triplet* curr_job;
    int jobs;

    jobs = concurrencyLevel - c_executing.jobs_in_queue;
    if(jobs>0){         //more jobs can be added in the executing queue
        for(int i=1;i<=jobs;i++){
            if(buffer.jobs_in_queue > 0){
                curr_job = Dequeue(&buffer);
                Exec_Enqueue(&c_executing,curr_job);    
            }
        }
    }else{              //some jobs must be removed from the executing queue
        jobs = c_executing.jobs_in_queue - concurrencyLevel;
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

char** Create_Array_of_args(char* command){
    char temp1[100],temp2[100];
    strcpy(temp1,command);
    strcpy(temp2,command);
    const char delim[] = " ";
    char **args;
    char *token = strtok(temp1, delim);
    int count;
    // Iterate through the tokens
    while (token != NULL) {
        count++;
        token = strtok(NULL, delim); // Get the next token
    }
    //printf("Count is %d\n",count);
    args = malloc(count * sizeof(char*));

    // Tokenize the string
    token = strtok(temp2, delim);
    //printf("token is %s\n",token);
    args[0] = malloc((strlen(token)+1)*sizeof(char));
    strcpy(args[0],token);
    //printf("lalalalla %s\n",args[0]);
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
    // printf("%s\n",args[0]);
    // for(int i=0;i<count;i++){
    //     printf("%s\n",args[i]);
    // }
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