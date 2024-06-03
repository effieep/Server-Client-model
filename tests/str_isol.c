#include <stdio.h>
#include <string.h>
#include <stdlib.h>


char** Create_Array_of_args(char* command){
    char temp1[100],temp2[100];
    strcpy(temp1,command);
    strcpy(temp2,command);
    const char delim[] = " ";
    char **args;
    char *token = strtok(temp1, delim);
    int count = 0;
    // Iterate through the tokens
    while (token != NULL) {
        count++;
        token = strtok(NULL, delim); // Get the next token
    }
    printf("Count is %d\n",count);
    args = malloc(count * sizeof(char*));

    // Tokenize the string
    token = strtok(temp2, delim);
    printf("token is %s\n",token);
    args[0] = malloc((strlen(token)+1)*sizeof(char));
    strcpy(args[0],token);
    printf("allalalalal args[0] %s\n",args[0]);
    count = 1;
    while (token != NULL) {
        token = strtok(NULL, delim); // Get the next token
        if(token != NULL){
            args[count] = malloc((strlen(token)+1)*sizeof(char));
            strcpy(args[count],token);
            count++;
        }
    }
    args[count] = NULL;
    for(int i=0; i<count; i++){
        printf("%s\n",args[i]);
    }
    return args;
}

int main(void){
    char command[]="issueJob ls -l";
    char* result = malloc(10 * sizeof(char));
    strcpy(result,"");
    const char delim[] = " ";

    char *token = strtok(command, delim);
    while (token != NULL) {
        token = strtok(NULL, delim); // Get the next token
        if(token != NULL){
            strcat(result,token);
            strcat(result," ");
        }
    }
    result[strlen(result) - 1] = '\0';
    printf("result is :%s\n",result);
    Create_Array_of_args(result);
    return 0;
}