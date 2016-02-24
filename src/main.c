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
    
    int is_built_in=0;    
    char *p,*q;
    if(strcmp(c->args[0],"logout")==0)
        exit(0);
    if(strcmp(c->args[0],"pwd")==0){
        pwd();
        is_built_in=1;
    }
    if(strcmp(c->args[0],"cd")==0){
        if(c->nargs==1)
            cd(NULL);
        else 
            cd(c->args[1]);
        is_built_in=1;
    }
    if(strcmp(c->args[0],"echo")==0){
        is_built_in=1;
        if(c->out!=Tnil){
            switch(c->out){
                case Tout:
                case ToutErr:
                    echo((c->args)+1,c->outfile,0);
                    break;
                case Tapp:
                case TappErr:  
                    echo((c->args)+1,c->outfile,1);
                    break;
            }
        }else{
            echo((c->args)+1,NULL,0);
        }
    }
    if(strcmp(c->args[0],"setenv")==0){
        p=NULL;
        q=NULL;
        is_built_in=1;
        if(c->nargs>1)
            p=c->args[1];
        if(c->nargs>2)
            q=c->args[2];
        setenv_(p,q);
    }
    if(strcmp(c->args[0],"unsetenv")==0){
        p=NULL;
        q=NULL;
        is_built_in=1;
        if(c->nargs>1)
            p=c->args[1];
        else{
            printf("Error: Provide a name of the variable to be unset\n");
            return;
        }
        unsetenv_(p);
    }
    if(is_built_in) 
        return;     
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
                dispatch_cmd(c);
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

    init_environment();
    read_ushrc();
    run_shell_interactive(1,1);
    return 0;
}

