#include "help_client.h"


int main(int argc,char *argv[]){
    //Check the number of arguments
    if(argc < 4){
        printf("Usage: ./bin/jobCommander [serverName] [portNum] [jobCommanderInputCommand]\n");
        printf("Choices are: \n> issueJob <job>\n> setConcurrency <N>\n> stop <jobID>\n> poll\n> exit\n");
        exit(1);
    }
    //Check if the data input are valid
    int portnum = atoi(argv[2]);
    if(portnum<1024 || portnum>65535){
        printf("Wrong portNum...Choose a port between 1024 and 65535\n");
        exit(1);
    }
    if(strcmp(argv[3],"issueJob")!= 0 && strcmp(argv[3],"setConcurrency")!=0 && \
        strcmp(argv[3],"stop")!= 0 && strcmp(argv[3],"poll")!= 0 && strcmp(argv[3],"exit")!= 0){
        printf("Command does not exist!\n");
        printf("Choices are: \n> issueJob <job>\n> setConcurrency <N>\n> stop <jobID>\n> poll\n> exit\n");
        exit(1);
    }
    Connect_to_Server(argc,argv);
}