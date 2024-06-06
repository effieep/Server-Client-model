#include "help_server.h"
#include "queue.h"
#include "actions.h"

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cvar;           /* Condition variable */

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
    int i=0;
    while(n>0){
        char digit = n%10 + '0';
        n = n/10;
        str_num[i++] = digit;
    }
    str_num = string_reverse(str_num);
    str_num[strlen(str_num)] = '\0';
    return str_num;
}

void perror_exit(char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

void Write_to_Commander(int socketfd,char* buff){
    buff[strlen(buff)] = '\0'; 
    printf("String to be sent is %s\n",buff);
    int length = strlen(buff) + 1;
    printf("length %d\n",length);
    char* res = malloc((length+10) * sizeof(char));
    char* l = int_to_string(length);
    strcpy(res,"");
    strcat(res,l);
    strcat(res," ");
    strcat(res,buff);
    printf("Response is :%s\n",res);
    if(write(socketfd,res,strlen(res)+1) == -1){
        perror("write");
        exit(1);
    }
}

char* Read_from_Commander(int socketfd){
    char ch;
    int bytes_read;
    char *size = malloc(10*sizeof(char));
    int i = 0;
    while ((bytes_read = read(socketfd, &ch, 1)) > 0) {
        printf("char is : %d\n",ch);
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
    free(size);
    return buffer;
}


void *Controller_Thread(void* arg) {
    int newsock = *((int*)arg);
    printf("Argument passed is %d\n",newsock);
    char *command = Read_from_Commander(newsock);
    printf("Command read is: %s\n",command);
    switch_command(newsock,command);
    printf("Closing connection.\n");
    return NULL;
}

void *Worker_Thread(void *args){
    job_triplet* job = Read_Buffer();
    printf("Job retrieved is: %s %s,%d\n",job->job_id,job->command,job->client_socket);
    //RUN JOB
    Exec_Job(job);
    //RETURN OUTPUT
    return NULL;
}

void Create_WorkerThreads(int k){
    int err;
    pthread_t wthr;
    for(int i=1;i<=k;i++){
        if ((err = pthread_create(&wthr, NULL, Worker_Thread, NULL))!=0) { /* New thread */
            perror2("pthread_create", err);
            exit(1); 
        }
    }
}

void Accept_Clients(char** argv){
    int port, sock, newsock,buffersize,threadPoolSize;
    struct sockaddr_in server, client;
    socklen_t clientlen;

    struct sockaddr *serverptr=(struct sockaddr *)&server;
    struct sockaddr *clientptr=(struct sockaddr *)&client;

    port = atoi(argv[1]);
    buffersize = atoi(argv[2]);
    threadPoolSize = atoi(argv[3]);
    Initialize_buffer(buffersize);
    Cond_Initialization();
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
        printf("Accepted connection in socket %d\n",newsock);
        int err, status;
        pthread_t thr;
        int* arg = malloc(sizeof(int));
        *arg = newsock;
        printf("Creating the controller thread...\n");
        if ((err = pthread_create(&thr, NULL, Controller_Thread, arg))!=0) { /* New thread */
            perror2("pthread_create", err);
            exit(1); 
        }
        pthread_t wthr;
        for(int i=1;i<=threadPoolSize;i++){
            if ((err = pthread_create(&wthr, NULL, Worker_Thread, NULL))!=0) { /* New thread */
                perror2("pthread_create", err);
                exit(1); 
            }
        }

        //Create_WorkerThreads(threadPoolSize);

        printf("I am original thread %ld and I created thread %ld and %ld\n", 
            (unsigned long)pthread_self(), (unsigned long)thr,(unsigned long)wthr);
        if ((err = pthread_join(wthr, (void **) &status))!=0) { /* Wait for thread */
            perror2("pthread_join", err); /* termination */
            exit(1); 
        }
        else printf("Just joinned the two threads ->one!\n");  
        if ((err = pthread_join(thr, (void **) &status))!=0) { /* Wait for thread */
            perror2("pthread_join", err); /* termination */
            exit(1); 
        }
        close(newsock);
    }
    close(sock);
}