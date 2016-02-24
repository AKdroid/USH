#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<unistd.h>
#include<fcntl.h>
#include"parse.h"
#include"builtins.h"
#define HOST "arrao-ush"

void dispatch_cmd(Cmd c){
    
    if(strcmp(c->args[0],"end") ==0 || strcmp(c->args[0],"logout")==0)
        exit(0);
    
}

void run_shell_interactive(int print,int exit_on_end){
    char* hostname = HOST;
    Pipe p;
    Cmd c;
    int i;
    while(1){
        if(print!=0)
            printf("%s %% ",hostname);
        fflush(stdout);
        p = parse();
        if(p!=NULL){
            c=p->head;
            if(c!=NULL){
                if(strcmp(c->args[0],"end") ==0)
                    break;
                printf("%s\n",c->args[0]);
            }
        }
    }
    fflush(stdout);
    if(exit_on_end)
        exit(0);
}

void read_ushrc(){
    
    int stdin_copy;
    int stdout_copy;
    int ushrc_fp,tmp;
    char* home,ushrc[200];
    stdin_copy=dup(STDIN_FILENO);
    stdout_copy=dup(STDOUT_FILENO);
    close(STDOUT_FILENO);
    ushrc[0]=0;
    home=getenv("HOME");
    strcat(ushrc,home);
    strcat(ushrc,"/.ushrc");
    ushrc_fp=open(ushrc,O_RDONLY);
    tmp = open(".ushrc.log",O_WRONLY | O_CREAT | O_TRUNC);
    if(ushrc_fp<0)
        return;
    dup2(ushrc_fp,STDIN_FILENO);
    //dup2(tmp,STDOUT_FILENO);
    run_shell_interactive(0,0);
    dup2(stdin_copy,STDIN_FILENO);
    dup2(stdout_copy,STDOUT_FILENO);
}

int main(){

    read_ushrc();
    run_shell_interactive(1,1);
    return 0;
}

