#include "help_client.h"


int main(int argc,char *argv[]){
    
    if(argc < 4){
        printf("Usage: ./bin/jobCommander [serverName] [portNum] [jobCommanderInputCommand]\n");
        printf("Choices are: \n> issueJob <job>\n> setConcurrency <N>\n> stop <jobID>\n> poll\n> exit\n");
        exit(0);
    }
    int portnum = atoi(argv[2]);
    if(portnum<1024 || portnum>65535){
        printf("Wrong portNum...Choose a port between 1024 and 65535\n");
        exit(EXIT_FAILURE);
    }
    if(strcmp(argv[3],"issueJob")!= 0 && strcmp(argv[3],"setConcurrency")!=0 && \
        strcmp(argv[3],"stop")!= 0 && strcmp(argv[3],"poll")!= 0 && strcmp(argv[3],"exit")!= 0){
        printf("Command does not exist!\n");
        printf("Choices are: \n> issueJob <job>\n> setConcurrency <N>\n> stop <jobID>\n> poll\n> exit\n");
        exit(0);
    }
    Connect_to_Server(argc,argv);
}