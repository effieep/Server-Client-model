#include "help.h"

char *workfile="jobExecutorServer.txt";

int pid_mining();
bool server_exists(char* filename);

int main(int argc,char *argv[]){
    int status;
    char buff[100];
    pid_t serverid,commanderid;       
    
    if(argc < 2){
        printf("Usage: ./JobCommander {choice} {arguments}\n");
        printf("Choices are: \n> issueJob <job>\n> setConcurrency <N>\n> stop <jobID>\n> poll [running,queued]\n> exit\n");
        exit(0);
    }
    //Checking if server is already activated by the existence of jobexecutorserver.txt file
    bool already_exists = server_exists(workfile);
    if(!already_exists){
        serverid=fork();
        if(serverid < 0){
            perror("fork");
        }else if(serverid == 0){
            char* args[] = {"JobExecutorServer", NULL};
            if (execv("JobExecutorServer", args) == -1) {
                perror("execv");
                exit(1);
            }
        }
    }else{
        //send signal to awake server
        serverid = pid_mining();
        //printf("Server already activated with ProcessId %d\n",serverid);
        kill(serverid,SIGUSR1);
    }
    if(serverid != 0){
        //code that runs every time that jobCommander is executed
        Write_to_Server(argc,argv);
        switch_command_C(argv[1]);
    }
}

void Read_from_Server(char* fifo_name){
    char buff2[BUFF_SIZE];
    if(mkfifo(fifo_name,0644) == -1){
            perror("mkfifo");
            exit(1);
    }
    int fd1;
    if((fd1 = open(fifo_name,O_RDONLY)) == -1){
        perror("open");
        exit(1);
    }
    int bytes_read = read(fd1, buff2, sizeof(buff2));
    printf("%s \n",buff2);
    close(fd1);
    //Delete the fifo file
    if (unlink(fifo_name) == -1) {
        perror("unlink"); 
        exit(1);
    }
}

void switch_command_C(char* tok){
    if(strcmp(tok,"issueJob")== 0){
        Read_from_Server("issue");
    }else if(strcmp(tok,"stop")==0){
        Read_from_Server("stop");
    }else if(strcmp(tok,"poll")==0){
        Read_from_Server("poll");
    }else if(strcmp(tok,"exit")==0){
        Read_from_Server("exit");
    }
}

void Write_to_Server(int argc,char** argv){
    char buff1[BUFF_SIZE] = "";
    buff1[0] = '\0';
    for(int i=1;i<argc;i++){
        strcat(buff1, argv[i]);
        if(i < argc-1){
            strcat(buff1," ");    //don't add space character at the end
        }
    }
    buff1[strlen(buff1)] = '\0'; 
    int wr,fd;
    sleep(1);
    if( (fd = open("myfifo",O_WRONLY)) == -1){
        perror("open");
    }
    if(write(fd, buff1, strlen(buff1)+1) == -1){
        perror("write");
        exit(5);
    }
    close(fd);
}

bool server_exists(char *filename){
    struct stat buf;
    if(stat(filename,&buf) == -1){
        if(errno == ENOENT){
            printf("file does not exists\n");
            return false;
        }
    }
    return true;
}

int pid_mining(){
    char buf[10];
    int fd = open(workfile, O_RDONLY, 0644);
    if(fd<0){
        perror("open");
    }else{
        int bytes_read = read(fd,buf,sizeof(buf));
    }
    return atoi(buf);
}