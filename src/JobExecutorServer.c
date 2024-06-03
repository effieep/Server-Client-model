#include "help_server.h"
#include "queue.h"

int main(int argc,char *argv[]){
    
    if(argc < 4){
        printf("Usage: ./bin/jobExecutorServer [portNum] [bufferSize] [threadPoolSize]\n");
        exit(0);
    }
    int portnum = atoi(argv[1]);
    int bufferSize = atoi(argv[2]);
    int threadPoolSize = atoi(argv[3]);
    if(portnum<1024 || portnum>65535){
        printf("Wrong portNum...Choose a port between 1024 and 65535\n");
    }else if(bufferSize<=0){
        printf("Wrong bufferSize...\n");
    }else if(threadPoolSize<=0){
        printf("Wrong input...\n");
    }
    Accept_Clients(argv);
}