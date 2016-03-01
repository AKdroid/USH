#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<unistd.h>
#include<fcntl.h>
#include<signal.h>
#include"parse.h"
#include"builtins.h"
#include"redirect.h"
#include"jobs.h"

/*
    Author: Akhil Raghavendra Rao (arrao@ncsu.edu)
    Main Driver Program For USH -> Micro Shell
    CSC501 Assignment 2
    
    Initialize the shell and make it fg.
    
    Read the $HOME/.ushrc to initialize
    
    Provide the interactive shell
*/

void run_shell_interactive(int print,int exit_on_end){

    /*
     * runs the interactive shell
     */ 
    char hostname[60];
    gethostname(hostname,60); 
    Pipe p,onext;
    Cmd c;
    int i,fp;
    int *in_pipe=NULL,*out_pipe=NULL;
    while(1){
        update_job_status();
        fflush(stdout);
        if(print!=0)
            printf("%s%% ",hostname);
        fflush(stdout);
        p = parse();
        while(p!=NULL){
            onext=p->next;
            p->next=NULL;
            if(create_job(p,0,0,0)==0){
                goto exitloop;
            }
            p=onext;
        }
    }
    exitloop:
    fflush(stdout);
}

void read_ushrc(){
    /*
     * Reads the executes the .ushrc file
     */ 
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
    if(ushrc_fp>=0){
        tmp = open(".ushrc.log",O_WRONLY | O_CREAT | O_TRUNC , S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        dup2(ushrc_fp,STDIN_FILENO);
        dup2(tmp,STDOUT_FILENO);
        dup2(tmp,STDERR_FILENO);
        run_shell_interactive(0,0);
    }
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
    job_cleanup();
    return 0;
}

