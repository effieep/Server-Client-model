#include "help_server.h"
#include "queue.h"
#include "actions.h"

pthread_mutex_t bmtx = PTHREAD_MUTEX_INITIALIZER;           //buffer mutex
pthread_mutex_t emtx = PTHREAD_MUTEX_INITIALIZER;           //mutex to lock when a job is executing 
pthread_cond_t empty,fill,concurr;

int executing = 0;
bool restart = 1;

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
    exit(1);
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
        exit(1);
    }
    free(size);
    return buffer;
}


void *Controller_Thread(void* arg) {
    ctrl_args *data;
    data = (ctrl_args *) arg;
    int newsock = data->socket;
    pthread_t* workers_id = data->wth;
    int threads = data->threads;
    printf("Workers ids are:\n");
    for(int i=0;i<2;i++){
        printf("id : %ld\n",(unsigned long)workers_id[i]);
    }
    printf("Argument passed is %d\n",newsock);
    char *command = Read_from_Commander(newsock);
    printf("Command read is: %s\n",command);
    switch_command(newsock,command,workers_id,threads);
    free(command);
    printf("Exit Controller Thread\n");
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
    printf("Sending signal fill\n");
    pthread_cond_signal(&fill);
    if((err = (pthread_mutex_unlock(&bmtx))) < 0){
        perror2("mutex_unlock",err);
    }
}

job_triplet* Read_Buffer(){
    int err;
    if((err = (pthread_mutex_lock(&bmtx))) < 0){
        perror2("mutex_lock",err);
    }
    control* buff = get_buffer();
    while(buff->jobs_in_queue == 0){
        printf("Buffer Empty case: Thread %ld in waiting stage\n",(unsigned long)pthread_self());
        pthread_cond_wait(&fill,&bmtx);
        printf("read buffer thread %ld restart is now : %d\n",(unsigned long)pthread_self(),restart);
        if(restart == 0) return NULL;
    }
    
    printf("Thread %ld woke up to read a job from buffer!\n",(unsigned long)pthread_self());
    if(executing > getconcurrency()) return NULL;
    job_triplet* job = Dequeue(buff);
    if((err = (pthread_mutex_unlock(&bmtx))) < 0){
        perror2("mutex_unlock",err);
    }
    printf("Sending signal empty\n");
    pthread_cond_signal(&empty);        //there is free space now in the buffer so a new job can be added
    return job;
}

void lock(){
    int err;
    if((err = (pthread_mutex_lock(&emtx))) < 0){
        perror2("mutex_lock",err);
    }
}

void unlock(){
    int err;
    if((err = (pthread_mutex_unlock(&emtx))) < 0){
        perror2("mutex_unlock",err);
    }
}

void broadcast_fill(){
    int err;
    if((err = (pthread_mutex_lock(&bmtx))) < 0){
        perror2("mutex_lock",err);
    }
    if((err = (pthread_cond_broadcast(&fill))) < 0){
        perror2("broadcast",err);
    }
    printf("Sent broadcast to fill\n");
    if((err = (pthread_mutex_unlock(&bmtx))) < 0){
        perror2("mutex_lock",err);
    }
}

void broadcast_concurr(){
    int err;
    if((err = (pthread_mutex_lock(&emtx))) < 0){
        perror2("mutex_lock",err);
    }
    if((err = (pthread_cond_broadcast(&concurr))) < 0){
        perror2("broadcast",err);
    }
    printf("Sent broadcast to concurr\n");
    if((err = (pthread_mutex_unlock(&emtx))) < 0){
        perror2("mutex_unlock",err);
    }
}

void Inform_Worker_Threads(int concurrency){
    if(concurrency > executing){
        int enable = concurrency - executing;
        for(int i=0;i<enable;i++){
            pthread_cond_signal(&concurr);
        }
    }
}

void Disable_restart(){
    int err;
    restart = 0;
    printf("Restart set to 0\n");
}

void Check_concurrency(){
    int err;
    if((err = (pthread_mutex_lock(&emtx))) < 0){
        perror2("mutex_lock",err);
    }
    while(executing == getconcurrency()){
        printf("Concurrency reached case: Thread %ld in waiting stage\n",(unsigned long)pthread_self());
        pthread_cond_wait(&concurr,&emtx);
        printf("main thread %ld restart is now : %d\n",(unsigned long)pthread_self(),restart);
        if(restart == 0) break;
    }
    if((err = (pthread_mutex_unlock(&emtx))) < 0){
        perror2("mutex_unlock",err);
    }
}

void *Worker_Thread(void *args){
    int err;
    do{
        Check_concurrency();
        if(restart == 0) break;            //exit command was sent
        if((err = (pthread_mutex_lock(&emtx))) < 0){
            perror2("mutex_lock",err);
        }
        executing++;
        printf("Before exec : %d\n",executing);
        if((err = (pthread_mutex_unlock(&emtx))) < 0){
            perror2("mutex_unlock",err);
        }
       
        job_triplet* job = Read_Buffer();
        if(job == NULL) continue;           //concurrency became smaller
        if(restart == 0) break;             //exit command was sent
        printf("Job retrieved is: %s %s,%d\n",job->job_id,job->command,job->client_socket);
        //Execute the job
        Exec_Job(job);
        printf("Closing connection.\n");
        close(job->client_socket);
        if((err = (pthread_mutex_lock(&emtx))) < 0){
            perror2("mutex_lock",err);
        }
        executing--;
        printf("After exec : %d\n",executing);
        if((err = (pthread_mutex_unlock(&emtx))) < 0){
            perror2("mutex_unlock",err);
        }
    }while(restart);
    printf("Worker Thread %ld exited\n",(unsigned long)pthread_self());
    pthread_exit(NULL);
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
    pthread_t workers[threadPoolSize];
    for(int i=0;i<threadPoolSize;i++){
        if ((err = pthread_create(&workers[i], NULL, Worker_Thread, NULL))!=0) { /* New thread */
            perror2("pthread_create", err);
            exit(1); 
        }
    }

    while (1) {
        printf("Waiting for connection...\n");
        /* accept connection */
    	if ((newsock = accept(sock, clientptr, &clientlen)) < 0) perror_exit("accept");  
        printf("Accepted connection in socket %d\n",newsock);
        int status;
        pthread_t thr;
        ctrl_args arg;
        arg.socket = newsock;
        arg.wth = workers;
        arg.threads = threadPoolSize;
        printf("Creating the controller thread...\n");
        if ((err = pthread_create(&thr, NULL, Controller_Thread, &arg))!=0) { /* New thread */
            perror2("pthread_create", err);
            exit(1); 
        }
        //Release its sources when it will terminate
        if ((err = pthread_detach(thr))!=0) { 
            perror2("pthread_create", err);
            exit(1); 
        }
    }
    close(sock);
}