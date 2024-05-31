#include "help_client.h"

char *workfile="jobExecutorServer.txt";

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

void Connect_to_Server(int argc,char** argv){
    int portnum, sockfd, i;
    char buf[256];

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
    if (connect(sockfd, serverptr, sizeof(server)) < 0)
	   perror("connect");
    printf("Connecting to %s port %d\n", argv[1], portnum);
    do {
    	printf("Give input string: ");
    	fgets(buf, sizeof(buf), stdin);	                        /* Read from stdin*/
    	for(i=0; buf[i] != '\0'; i++) {                         /* For every char */
    	    /* Send i-th character */
        	if (write(sockfd, buf + i, 1) < 0){
                perror("write");
                exit(EXIT_FAILURE);
            }
        	   
            /* receive i-th character transformed */
        	if (read(sockfd, buf + i, 1) < 0){
                perror("read");
                exit(EXIT_FAILURE);   
            }
    	}
    	printf("Received string: %s", buf);
    } while (strcmp(buf, "END\n") != 0);                       /* Finish on "end" */
    close(sockfd);
}






// bool server_exists(char *filename){
//     struct stat buf;
//     if(stat(filename,&buf) == -1){
//         if(errno == ENOENT){
//             printf("file does not exists\n");
//             return false;
//         }
//     }
//     return true;
// }