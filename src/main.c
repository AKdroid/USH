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
#define HOST "arrao-ush"

void dispatch_cmd(Cmd c, int nice, int shift, int nv, int* in_pipe, int* out_pipe);

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


void backup_fp(Cmd c, int* infp_bk, int* outfp_bk, int* errfp_bk){
    if(infp_bk != NULL){
        if(c->in != Tnil) 
            *infp_bk = dup(STDIN_FILENO);
        else
            *infp_bk=-1;
    }
    if(outfp_bk!= NULL){
        if(c->out != Tnil)
            *outfp_bk=dup(STDOUT_FILENO);
        else
            *outfp_bk=-1;
    }
    if(errfp_bk != NULL){
        if(c->out == TappErr || c->out == ToutErr || c->out == TpipeErr)
            *errfp_bk=dup(STDERR_FILENO);
        else   
            *errfp_bk=-1;
    }    
}

void restore_fp(int infp_bk, int outfp_bk, int errfp_bk){
    if(infp_bk>=0)
        dup2(infp_bk,STDIN_FILENO);
    if(outfp_bk>=0)
        dup2(outfp_bk,STDOUT_FILENO);
    if(errfp_bk>=0)
        dup2(errfp_bk,STDERR_FILENO);
}

void set_redirections(Cmd c, int infp, int outfp){
    if(c->in != Tnil && infp >=0)
        redirect_stdin(infp);
    if(outfp>=0 && (c->out== Tout || c->out == Tapp || c->out == Tpipe))
        redirect_stdout(outfp);
    if(outfp>=0 && (c->out== ToutErr || c->out == TappErr || c->out == TpipeErr))
        redirect_stdout_err(outfp);
}

void create_subshell(Cmd c, int nice, int nicevalue, int shift, int infp, int outfp){
    int status;
    pid_t pid;
    char* cmd=c->args[shift];
    int exec_result;
    pid=fork();
    if(pid < 0){
        fputs("Error: Fork failed\n",stderr);
    } else if (pid == 0){
        set_redirections(c,infp,outfp);
        if(nice)
            nice_(nicevalue);
        if(is_built_in(cmd)){
            if(strcmp(c->args[shift],"nice")!=0)
                c->out=Tnil;
            dispatch_cmd(c,0,shift,nicevalue,NULL,NULL);
            exit(0);
        }else{
            exec_result=execvp(cmd,(c->args)+shift);
            if(exec_result<0)
                exit(1);
        }
    } else {
        if(infp >=3)
            close(infp);
        if(outfp >=3)
            close(outfp);
        if(c->exec == Tamp){
            printf("[%d] %d\n",1,pid);
        }else {
            wait(&status);
        }
    }
}

void open_files_for_redirection(Cmd c,int* in_pipe, int* out_pipe, int* infp, int* outfp ){
    *infp=-1;
    *outfp=-1;
    switch(c->in){
        case Tin:
            *infp=open_file_for_read(c->infile);
            break;
        case Tpipe:
        case TpipeErr:
            if(in_pipe!=NULL)
                *infp = in_pipe[0];
            break;
    }
    switch(c->out){
        case Tout:
        case ToutErr:
            *outfp=open_file_for_write(c->outfile);
            break;
        case Tapp:
        case TappErr:
            *outfp=open_file_for_append(c->outfile);
            break;
        case Tpipe:
        case TpipeErr:
            if(out_pipe!=NULL)
                *outfp=out_pipe[1];
             break;
    }
}

void dispatch_cmd(Cmd c, int nice, int shift, int nice_value, int* in_pipe, int* out_pipe){
    
    int nv=0,valid=0;    
    int spawn_subshell;
    char *p,*q;
    int infp, outfp, infp_bk, outfp_bk, errfp_bk;
    if(strcmp(c->args[shift],"nice")!=0)
        open_files_for_redirection(c,in_pipe,out_pipe,&infp,&outfp);
    if(nice){
        create_subshell(c,1,nice_value,shift,infp,outfp);
        return;
    }
//    open_files_for_redirection(c,in_pipe,out_pipe,&infp,&outfp);
    spawn_subshell=is_built_in(c->args[shift]);
    if(spawn_subshell==0){
        create_subshell(c,0,0,0,infp,outfp);
        return;
    }
    if(spawn_subshell==1 && (c->out==Tpipe || c->out==TpipeErr)){
        fflush(stdout);
        create_subshell(c,nice,nice_value,shift,infp,outfp);
        return;
    }
    else if(spawn_subshell == 1 && (c->in != Tnil || c->out != Tnil )){
        backup_fp(c,&infp_bk,&outfp_bk,&errfp_bk);
        set_redirections(c,infp,outfp);
    }/*
    if(nice){
        printf("In execution of nice next\n");
        create_subshell(c,1,nice_value,shift,infp,outfp);
        return;
    }*/
    if(strcmp(c->args[shift],"logout")==0)
        exit(0);
    if(strcmp(c->args[shift],"pwd")==0){
        pwd();
    }
    if(strcmp(c->args[shift],"cd")==0){
        if(c->nargs==1+shift)
            cd(NULL);
        else 
            cd(c->args[shift+1]);
    }
    if(strcmp(c->args[shift],"echo")==0){
        /*
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
        }else{*/
            echo((c->args)+1+shift);//,NULL,0);
        
    }
    if(strcmp(c->args[shift],"setenv")==0){
        p=NULL;
        q=NULL;
        if(c->nargs>1+shift)
            p=c->args[1+shift];
        if(c->nargs>2+shift)
            q=c->args[shift+2];
        setenv_(p,q);
    }
    if(strcmp(c->args[shift],"unsetenv")==0){
        p=NULL;
        q=NULL;
        if(c->nargs>1+shift)
            p=c->args[1+shift];
        else{
            printf("Error: Provide a name of the variable to be unset\n");
            return;
        }
        unsetenv_(p);
    }
    if(strcmp(c->args[shift],"where")==0){
        if(c->nargs!=2+shift){
            fputs("Error: where requires 1 argument\n",stderr);
        }else{
            where(c->args[1+shift]);
        }
    }
    if(strcmp(c->args[shift],"nice")==0){
        if(c->nargs<=shift+1){
            fputs("Error: nice requires at least 1 argument\n",stderr);
        }else{
            nv=(int)is_numeric(c->args[shift+1],&valid);
            //if(infp>=0)close(infp);
            //if(outfp>=0)close(outfp);
            if(valid){
                if(c->nargs>=shift+3)
                    dispatch_cmd(c,1,shift+2,nv,in_pipe,out_pipe);
            }else{
                dispatch_cmd(c,1,shift+1,4,in_pipe,out_pipe);
            }
        }
    }
    
    if(spawn_subshell == 1 && (c->in != Tnil || (c->out != Tnil && c->out != Tpipe && c->out !=TpipeErr))){
        fflush(stdin);
        fflush(stdout);
        fflush(stderr);
        restore_fp(infp_bk,outfp_bk,errfp_bk);
    }
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
            c=p->head;
            while(c!=NULL){
                if(strcmp(c->args[0],"end") ==0)
                    goto exitloop;
                if(c->in != Tnil){
                    fp=open(c->infile,O_RDONLY);
                    if(fp == -1){
                        printf("file not found: %s\n",c->infile);
                        break;
                    }
                }
                in_pipe=out_pipe;
                //printf("%s\n",c->args[0]);
                if(c->out==Tpipe){
                    out_pipe=create_pipe();
                }else{
                    out_pipe=NULL;
                }
                dispatch_cmd(c,0,0,4,in_pipe, out_pipe);
                if(in_pipe != NULL)
                    destroy_pipe(in_pipe);
                c=c->next;
            }
            p=p->next;
        }
        freePipe(p);
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

