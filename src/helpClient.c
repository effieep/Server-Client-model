#include "help_client.h"

void swap_chars(char* ch1,char* ch2){
    char *tmp = NULL;
    *tmp = *ch1;
    *ch1 = *ch2;
    *ch2 = *tmp;
}

char* string_reverse(char *str){
    int front_index,end_index,length;
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
    memset(str_num,'\0',10*sizeof(char));
    int i=0;
    while(n>0){
        char digit = n%10 + '0';
        n = n/10;
        str_num[i++] = digit;
    }
    str_num = string_reverse(str_num);
    return str_num;
}

void Read_from_Server(int socketfd){
    char ch;
    int bytes_read;
    char *size = malloc(10*sizeof(char));
    int i = 0;
    while ((bytes_read = read(socketfd, &ch, 1)) > 0) {
        if (ch == ' ' || ch == '\n') {
            break;
        }
        size[i] = ch;
        i++;
    }
    int s = atoi(size);
    printf("size is %d;\n",s);
    char *buffer = malloc(s *sizeof(char));
    if(read(socketfd,buffer,s) == -1){
        perror("read");
        exit(EXIT_FAILURE);
    }
    printf("%s\n",buffer);
}

void Write_to_Server(int socketfd,int argc,char** argv){
   int sum = 0;
    for(int i=3;i<argc;i++){
        sum = sum + strlen(argv[i]) + 1;    //1 for a space character for every argument
    }
    char *buff1 = malloc((sum+10) * sizeof(char));  //+10 for the max 10 digits of number
    memset(buff1,'\0',sum*sizeof(char));
    char* l = int_to_string(sum);
    strcat(buff1,l);
    strcat(buff1," ");
    for(int i=3;i<argc;i++){
        strcat(buff1, argv[i]);
        if(i < argc-1){
            strcat(buff1," ");    //don't add space character at the end
        }
    }
    printf("String to be sent is %s\n",buff1);
    if(write(socketfd, buff1,strlen(buff1)+1) == -1){
        perror("write");
        exit(1);
    }
}

void Connect_to_Server(int argc,char** argv){
    int portnum, sockfd;

    struct sockaddr_in server;
    struct sockaddr *serverptr = (struct sockaddr*)&server;
    struct hostent *rem;

    /* Create socket */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){        //sock -> file descriptor of socket
        perror("socket");
        exit(EXIT_FAILURE);
    }

	/* Find server address */
    if ((rem = gethostbyname(argv[1])) == NULL) {	
	   herror("gethostbyname"); exit(1);
    }
    portnum = atoi(argv[2]);                                    /*Convert port number to integer*/
    server.sin_family = AF_INET;                                /* Internet domain */
    memcpy(&server.sin_addr, rem->h_addr, rem->h_length);
    server.sin_port = htons(portnum);                           /* Server port */

    /* Initiate connection */
    if (connect(sockfd, serverptr, sizeof(server)) < 0){
        perror("connect");
    }
    printf("Connecting to %s port %d\n", argv[1], portnum);
    printf("Client socket is : %d",sockfd);
    Write_to_Server(sockfd,argc,argv);
    Read_from_Server(sockfd);
    //select read and check file descriptors its time
    if(strcmp(argv[3],"issueJob")== 0){
        Read_from_Server(sockfd);
    }
    close(sockfd);
}
