#include "help_server.h"
#include "queue.h"
#include "actions.h"

void swap_chars(char* ch1,char* ch2){
    char *tmp;
    *tmp = *ch1;
    *ch1 = *ch2;
    *ch2 = *tmp;
}

char* string_reverse(char *str){
    int front_index,end_index,length;
    char temp;
    length = strlen(str);
    front_index = 0;
    end_index = length - 1;

    if(length%2 == 0){
        while(front_index<end_index){
            //swap front and end char
            char tmp = str[front_index];
            str[front_index] = str[end_index];
            str[end_index] = tmp;
            front_index++;
            end_index--;
        }
    }else{
        while(front_index!=end_index){
            //swap front and end char
            char tmp = str[front_index];
            str[front_index] = str[end_index];
            str[end_index] = tmp;
            front_index++;
            end_index--;
        }
    }
    return str;
}

char* int_to_string(int n){
    char *str_num = malloc(10*sizeof(char));
    int i=0;
    while(n>0){
        char digit = n%10 + '0';
        n = n/10;
        str_num[i++] = digit;
    }
    str_num = string_reverse(str_num);
    return str_num;
}
void perror_exit(char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

void Write_to_Commander(int socketfd,char* buff){
    if(write(socketfd, buff,strlen(buff)+1) == -1){
        perror("write");
        exit(1);
    }
}

void Read_from_Commander(int socketfd,char* command){
    if(read(socketfd, command,BUFF_SIZE) == -1){
        perror("read");
        exit(EXIT_FAILURE);
    }
}

void Interact(int newsock) {
    char command[BUFF_SIZE] = "";
    memset( command, '\0', sizeof(command)/sizeof(command[0]));
    Read_from_Commander(newsock,command);
    printf("Command read is: %s\n",command);
    switch_command(newsock,command);
    printf("Closing connection.\n");
    close(newsock);
}

void Accept_Clients(int argc,char** argv){
    int port, sock, newsock;
    struct sockaddr_in server, client;
    socklen_t clientlen;

    struct sockaddr *serverptr=(struct sockaddr *)&server;
    struct sockaddr *clientptr=(struct sockaddr *)&client;
    struct hostent *rem;

    port = atoi(argv[1]);
  
    /* Create socket */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        perror_exit("socket");
    server.sin_family = AF_INET;                        /* Internet domain */
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port);                      /* The given port */
    /* Bind socket to address */
    if (bind(sock, serverptr, sizeof(server)) < 0)
        perror_exit("bind");
    /* Listen for connections */
    if (listen(sock, 5) < 0) perror_exit("listen");
    printf("Listening for connections to port %d\n", port);
    while (1) {
        /* accept connection */
    	if ((newsock = accept(sock, clientptr, &clientlen)) < 0) perror_exit("accept");
        /* must be closed before it gets re-assigned */   
        printf("Accepted connection\n");
        Interact(newsock);  
	                          	      	   
    }
    close(sock);
}