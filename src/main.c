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

void dispatch_cmd(Cmd c, int nice, int shift, int nv);

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

void create_subshell(Cmd c, int nice, int nicevalue, int shift){
    int status;
    pid_t pid;
    char* cmd=c->args[shift];
    int exec_result;
    //Redirection and pipes
    //Code to be added
    pid=fork();
    if(pid < 0){
        fputs("Error: Fork failed",stderr);
    } else if (pid == 0){
        if(nice)
            nice_(nicevalue);
        if(is_built_in(cmd)){
            dispatch_cmd(c,0,shift,nicevalue);
        }else{
            exec_result=execvp(cmd,(c->args)+shift);
            if(exec_result<0)
                exit(1);
        }
    } else {
        if(c->exec == Tamp){
            printf("[%d] %d\n",1,pid);
        }else {
            wait(&status);
        }
    }
}

void dispatch_cmd(Cmd c, int nice, int shift, int nice_value){
    
    int is_built_in=0,nv=0,valid=0;    
    char *p,*q;
    if(nice){
        create_subshell(c,1,nice_value,shift);
        return;
    }
    if(strcmp(c->args[shift],"logout")==0)
        exit(0);
    if(strcmp(c->args[shift],"pwd")==0){
        pwd();
        is_built_in=1;
    }
    if(strcmp(c->args[shift],"cd")==0){
        if(c->nargs==1+shift)
            cd(NULL);
        else 
            cd(c->args[shift+1]);
        is_built_in=1;
    }
    if(strcmp(c->args[shift],"echo")==0){
        is_built_in=1;
        if(c->out!=Tnil){
            switch(c->out){
                case Tout:
                case ToutErr:
                    echo((c->args)+1+shift,c->outfile,0);
                    break;
                case Tapp:
                case TappErr:  
                    echo((c->args)+1+shift,c->outfile,1);
                    break;
            }
        }else{
            echo((c->args)+1+shift,NULL,0);
        }
    }
    if(strcmp(c->args[shift],"setenv")==0){
        p=NULL;
        q=NULL;
        is_built_in=1;
        if(c->nargs>1+shift)
            p=c->args[1+shift];
        if(c->nargs>2+shift)
            q=c->args[shift+2];
        setenv_(p,q);
    }
    if(strcmp(c->args[shift],"unsetenv")==0){
        p=NULL;
        q=NULL;
        is_built_in=1;
        if(c->nargs>1+shift)
            p=c->args[1+shift];
        else{
            printf("Error: Provide a name of the variable to be unset\n");
            return;
        }
        unsetenv_(p);
    }
    if(strcmp(c->args[shift],"where")==0){
        is_built_in=1;
        if(c->nargs!=2+shift){
            fputs("Error: where requires 1 argument\n",stderr);
        }else{
            where(c->args[1+shift]);
        }
    }
    if(strcmp(c->args[shift],"nice")==0){
        is_built_in=1;
        if(c->nargs<=shift+1){
            fputs("Error: nice requires at least 1 argument\n",stderr);
        }else{
            nv=(int)is_numeric(c->args[shift+1],&valid);
            if(valid){
                if(c->nargs>=shift+3)
                    dispatch_cmd(c,1,shift+2,nv);
            }else{
                dispatch_cmd(c,1,shift+1,4);
            }
        }
    }
    if(is_built_in) 
        return;     
    create_subshell(c,0,0,0);
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
        while(p!=NULL){
            c=p->head;
            if(c!=NULL){
                if(strcmp(c->args[0],"end") ==0)
                    goto exitloop;
                printf("%s\n",c->args[0]);
                dispatch_cmd(c,0,0,4);
            }
            p=p->next;
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

    init_environment();
    read_ushrc();
    run_shell_interactive(1,1);
    return 0;
}

