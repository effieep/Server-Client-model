#include "help_server.h"
#include "queue.h"
#include "actions.h"

pthread_mutex_t bmtx = PTHREAD_MUTEX_INITIALIZER;           //buffer mutex
pthread_mutex_t emtx = PTHREAD_MUTEX_INITIALIZER;           //mutex to lock when a job is executing 
pthread_cond_t empty,fill,concurr;

int executing = 0;

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
    str_num[strlen(str_num)] = '\0';
    return str_num;
}

void perror_exit(char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

void Write_to_Commander(int socketfd,char* buff){
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
    free(l);
    free(res);
}

char* Read_from_Commander(int socketfd){
    char ch;
    int bytes_read;
    char *size = malloc(10*sizeof(char));
    int i = 0;
    while ((bytes_read = read(socketfd, &ch, 1)) > 0) {
        printf("char is : %d\n",ch);
        if (ch == ' ') {
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
    free(command);
    free(arg); 
    printf("Closing connection.\n");
    pthread_exit(NULL);
}

void Cond_Initialization(){
    pthread_cond_init(&fill, NULL); /* Initialize condition variables */
    pthread_cond_init(&empty, NULL); 
    pthread_cond_init(&concurr, NULL); 
}

void Place_to_Buffer(int sockfd,char* command){
    int err;
    if((err = (pthread_mutex_lock(&bmtx))) < 0){
        perror2("mutex_lock",err);
    }
    control* buff = get_buffer();
    //If buffer is full put thread in sleeping mode 
    while(buff->jobs_in_queue == buff->max_jobs){
        printf("Buffer Full case: Thread %ld in waiting stage\n",(unsigned long)pthread_self());
        pthread_cond_wait(&empty,&bmtx);
    }
    printf("Thread %ld woke up to place a job to buffer!\n",(unsigned long)pthread_self());
    Enqueue(buff,command,sockfd);
    pthread_cond_signal(&fill);
    if((err = (pthread_mutex_unlock(&bmtx))) < 0){
        perror2("mutex_unlock",err);
    }
}

job_triplet* Read_Buffer(){
    control* buff = get_buffer();
    while(buff->jobs_in_queue == 0){
        printf("Buffer Empty case: Thread %ld in waiting stage\n",(unsigned long)pthread_self());
        pthread_cond_wait(&fill,&bmtx);
    }
    while(executing == getconcurrency()){
        printf("Concurrency reached case: Thread %ld in waiting stage\n",(unsigned long)pthread_self());
        pthread_cond_wait(&concurr,&bmtx);
    }
    printf("Thread %ld woke up to read a job from buffer!\n",(unsigned long)pthread_self());
    job_triplet* job = Dequeue(buff);
    pthread_cond_signal(&empty);        //there is free space now in the buffer so a new job can be added
    return job;
}

void lock(){
    int err;
    if((err = (pthread_mutex_lock(&bmtx))) < 0){
        perror2("mutex_lock",err);
    }
}

void unlock(){
    int err;
    if((err = (pthread_mutex_unlock(&bmtx))) < 0){
        perror2("mutex_unlock",err);
    }
}

void *Worker_Thread(void *args){
    bool restart = 1;
    int err;
    do{
        if((err = (pthread_mutex_lock(&bmtx))) < 0){
            perror2("mutex_lock",err);
        }
        job_triplet* job = Read_Buffer();
        if((err = (pthread_mutex_unlock(&bmtx))) < 0){
            perror2("mutex_unlock",err);
        }
        printf("Job retrieved is: %s %s,%d\n",job->job_id,job->command,job->client_socket);
        //Execute the job
        if((err = (pthread_mutex_lock(&emtx))) < 0){
            perror2("mutex_lock",err);
        }
        executing++;
        Exec_Job(job);
        printf("Closing connection.\n");
        close(job->client_socket);
        executing--;
        if((err = (pthread_mutex_unlock(&emtx))) < 0){
            perror2("mutex_unlock",err);
        }
        pthread_cond_signal(&concurr);
    }while(restart);
    return NULL;   
}

void Accept_Clients(char** argv){
    int err;
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

    // Create threadPoolSize worker threads
    // if((err = (pthread_mutex_lock(&mutex))) < 0){
    //     perror2("mutex_unlock",err);
    // }
    pthread_t workers[threadPoolSize];
    for(int i=0;i<threadPoolSize;i++){
        if ((err = pthread_create(&workers[i], NULL, Worker_Thread, NULL))!=0) { /* New thread */
            perror2("pthread_create", err);
            exit(1); 
        }
    }
    for(int i=0;i<threadPoolSize;i++){
        if ((err = pthread_detach(workers[i]))!=0) { /* New thread */
            perror2("pthread_create", err);
            exit(1); 
        }
    }
    // printf("Unlock shared data\n");
    // if((err = (pthread_mutex_unlock(&mutex))) < 0){
    //     perror2("mutex_unlock",err);
    // }

    while (1) {
        printf("Waiting for connection...\n");
        /* accept connection */
    	if ((newsock = accept(sock, clientptr, &clientlen)) < 0) perror_exit("accept");  
        printf("Accepted connection in socket %d\n",newsock);
        int status;
        pthread_t thr;
        int* arg = malloc(sizeof(int));
        *arg = newsock;
        printf("Creating the controller thread...\n");
        if ((err = pthread_create(&thr, NULL, Controller_Thread, arg))!=0) { /* New thread */
            perror2("pthread_create", err);
            exit(1); 
        }
        //Release its sources when it will terminate
        if ((err = pthread_detach(thr))!=0) { 
            perror2("pthread_create", err);
            exit(1); 
        }
        // //Wait for Controller Thread to end
        // if ((err = pthread_join(thr, (void **) &status))!=0) { /* Wait for thread */
        //     perror2("pthread_join", err); /* termination */
        //     exit(1); 
        // }else{
        //     printf("Controller thread completed its work!\n");
        // }
    
        //Create_WorkerThreads(threadPoolSize);
        //Main thread must wait the worker threads
        // printf("I am original thread %ld and I created thread %ld and %ld\n", 
        //     (unsigned long)pthread_self(), (unsigned long)thr,(unsigned long)workers[0]);
       
    }
    close(sock);
}