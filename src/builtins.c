#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include"builtins.h"

extern char** environ;

void pwd(){
    char path[512];
    if( getcwd(path,512) != 0)
        printf("%s\n",path);
    else
        printf("Error in getting current directory\n");
}

//Changes the directory to path if it exists and has permissions
int cd(char* path){
    
    int result;
    if(path == NULL )
        path = getenv("HOME");
    result = chdir(path);
    if(result != 0){
        printf("Error Occured while changing the directory\n");
        return -1;
    }
    return 0;
}

