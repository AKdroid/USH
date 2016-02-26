#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>
#include"parse.h"
#include"redirect.h"

int open_file_for_read(char* fname){
   
    int fp;
    fp = open(fname,O_RDONLY);
    return fp;
}

int open_file_for_write(char* fname){
    int fp;
    fp = open(fname,O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    return fp;
}

int open_file_for_append(char* fname){
    int fp;
    fp = open(fname,O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    return fp;
}

int* create_pipe(){
    int* pp = (int*)malloc(2*sizeof(int));
    pipe(pp);
    return pp;
}

void destroy_pipe(int *p){
    free(p);
}

int redirect_stdin(int infp){
    dup2(infp,STDIN_FILENO);
}

int redirect_stdout(int outfp){
    printf("Called redirect stdout\n");
    dup2(outfp, STDOUT_FILENO);
}

int redirect_stdout_err(int outfp){
    dup2(outfp,STDOUT_FILENO);
    dup2(outfp,STDERR_FILENO);
}
