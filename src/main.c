#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<unistd.h>
#include<fcntl.h>
#include"parse.h"
#include"builtins.h"
#include"redirect.h"
#include"jobs.h"
#define HOST "arrao-ush"


long int is_numeric(char* str, int* valid){
    char *eptr;
    long int ans=0;
    ans=strtol(str,&eptr,10);
    if(*eptr!='\0')
        *valid=0;
    else
        *valid=1;
    return ans;
}

void run_shell_interactive(int print,int exit_on_end){
    char* hostname = HOST;
    Pipe p;
    Cmd c;
    int i,fp;
    int *in_pipe=NULL,*out_pipe=NULL;
    while(1){
        if(print!=0)
            printf("%s %% ",hostname);
        fflush(stdout);
        p = parse();
        while(p!=NULL){
            create_job(p,0,0,0);
        }
    }
    exitloop:
    fflush(stdout);
    if(exit_on_end)
        exit(0);
}

void read_ushrc(){
    
    int stdin_copy;
    int stdout_copy,stderr_copy;
    int ushrc_fp,tmp;
    char* home,ushrc[200];
    stdin_copy=dup(STDIN_FILENO);
    stdout_copy=dup(STDOUT_FILENO);
    stderr_copy=dup(STDERR_FILENO);
    close(STDOUT_FILENO);
    ushrc[0]=0;
    home=getenv("HOME");
    strcat(ushrc,home);
    strcat(ushrc,"/.ushrc");
    ushrc_fp=open(ushrc,O_RDONLY);
    tmp = open(".ushrc.log",O_WRONLY | O_CREAT | O_TRUNC , S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if(ushrc_fp<0)
        return;
    dup2(ushrc_fp,STDIN_FILENO);
    dup2(tmp,STDOUT_FILENO);
    dup2(tmp,STDERR_FILENO);
    run_shell_interactive(0,0);
    fflush(stdout);
    fflush(stderr);
    dup2(stdin_copy,STDIN_FILENO);
    dup2(stdout_copy,STDOUT_FILENO);
    dup2(stderr_copy,STDERR_FILENO);
}

int main(){

    init_shell();
    init_environment();
    read_ushrc();
    run_shell_interactive(1,1);
    return 0;
}

