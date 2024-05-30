#include "help.h"
#include "queue.h"

//volatile sig_atomic_t keep_going = 1;
char *workfile="jobExecutorServer.txt";
static control c_waiting,c_executing;
static int concurrency = 1;

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

    //Remove the job from the executing queue that just terminated
    Remove_Pid(&c_executing,pid);
    job_triplet* new_job = Dequeue(&c_waiting);
    if(new_job != NULL){
        Exec_Enqueue(&c_executing,new_job);
    }
}

int main(void){
    int filedes;
    static struct sigaction act;
    bool execute = true;
    bool ready;
    mkfifo("myfifo",0666);

    Initialize_control_queue(&c_waiting);
    Initialize_control_queue(&c_executing);
    //Creation of the file jobExecutorServer.txt to determine if server is already activated
    pid_t pid = getpid();
    Create_Indicator(pid);

    act.sa_handler=handle_enable_server;
    //sigfillset(&(act.sa_mask));
	sigaction(SIGUSR1, &act, NULL);
    
    while(1){
        
        //Creating fifo for IPC
        char command[BUFF_SIZE] = "";
        memset( command, '\0', sizeof(command)/sizeof(command[0]));
        
        ready = Read_from_Commander(command);
        //check what should server do
        if(ready){
            switch_command(command);
        }else{
            continue;
        }
        if(c_waiting.jobs_in_queue > 0 || c_executing.jobs_in_queue > 0){
            Manage_Jobs();
        }
    }
    
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

bool Read_from_Commander(char* command){
    //returns false if there are no available data to read and true 
    //if a file descriptor is ready for reading
    fd_set rfds;
    struct timeval  tv;
    int check;
    int fd = open("myfifo",O_RDONLY | O_NONBLOCK);
    if (fd == -1) {
        perror("open");
        exit(1);
    }
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

    tv.tv_sec = 2;
    tv.tv_usec = 0;

    check = select(fd+1, &rfds, NULL, NULL, &tv);
    if(check == -1){            //error in select
        if (errno == EINTR) {
            // Select was interrupted by a signal, so retry
            printf("Interrupted by signal\n");
        } else {
            perror("select");
        }
        close(fd);
        return false;
    }else if(check){
        int bytes_read = read(fd, command,BUFF_SIZE);
        if(bytes_read == -1){
            perror("read");
            exit(7);
        }
        close(fd);
        return true;
    }else{                      //file descriptor not ready
        close(fd);
        //printf("No data avalaible to read\n");
        return false;
    }
}

void Create_Indicator(pid_t pid){
    int filedes;
    char* text = int_to_string(pid);
    printf("The process id of JobExecutorServer is %d %s\n",pid,text);

	if ( (filedes=open(workfile,  O_CREAT | O_RDWR | O_TRUNC, PERMS)) == -1){
		perror("creating");
		exit(1);
	}
	else { 
        if(write(filedes,text,strlen(text)) == -1){
            perror("write");
            exit(1);
        }
        if(close(filedes) == -1){
            perror("close");
            exit(1);
        }
	}
}

void Write_to_Commander(char* fifo_name,char* buff){
    int fd1;
    if((fd1 = open(fifo_name,O_WRONLY)) == -1){
        perror("comman open");
        exit(1);
    }
    if(write(fd1, buff,strlen(buff)+1) == -1){
        perror("write");
        exit(1);
    }
    if(close(fd1) == -1){
        perror("close");
        exit(1);
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
    Write_to_Commander("issue",buff);
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
    Write_to_Commander("exit",buff);
    //Destroy queue
    exit(0);
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
    Write_to_Commander("stop",buff);
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
    Write_to_Commander("poll",buff);
    free(buff);
}


void switch_command(char* comm){
    const char delim[] = " ";
    char* temp = malloc(BUFF_SIZE*sizeof(char));
    strcpy(temp,comm);
    char *tok = strtok(temp, delim);
    if(strcmp(tok,"issueJob")== 0){
        issueJob(comm);
    }else if(strcmp(tok,"setConcurrency")==0){
        char *num = strtok(NULL, delim); 
        concurrency = atoi(num);
        printf("Concurrency now is: %d\n",concurrency);
    }else if(strcmp(tok,"stop")==0){
        char *job = strtok(NULL, delim); 
        Stop_Job(job);
    }else if(strcmp(tok,"poll")==0){
        char *option = strtok(NULL, delim); 
        Poll(option);
    }else if(strcmp(tok,"exit")==0){
        Exit_Call();
    }else{
        printf("Command does not exists.Try again!\n");
    }
    free(temp);
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