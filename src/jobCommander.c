#include "help_client.h"

// int pid_mining();
// bool server_exists(char* filename);

int main(int argc,char *argv[]){
    int status;
    
    if(argc < 4){
        printf("Usage: ./bin/jobCommander [serverName] [portNum] [jobCommanderInputCommand]\n");
        printf("Choices are: \n> issueJob <job>\n> setConcurrency <N>\n> stop <jobID>\n> poll\n> exit\n");
        exit(0);
    }
    int portnum = atoi(argv[2]);
    if(portnum<1024 || portnum>65535){
        printf("Wrong portNum...Choose a port between 1024 and 65535\n");
    }
    Connect_to_Server(argc,argv);
}






































