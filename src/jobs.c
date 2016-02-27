#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<signal.h>
#include<termios.h>
#include"redirect.h"
#include"jobs.h"

int ush_pid;
int ush_terminal;
struct termios ush_modes;
int ush_interactive;

ush_job* head = NULL;

void disable_signals(){
    signal(SIGINT,SIG_IGN);
    signal(SIGTSTP,SIG_IGN);
    signal(SIGTTIN,SIG_IGN);
    signal(SIGTTOU,SIG_IGN);
    signal(SIGHUP,SIG_IGN);
    signal(SIGQUIT,SIG_IGN);
}

void enable_signals(){

    signal(SIGINT,SIG_DFL);
    signal(SIGTSTP,SIG_DFL);
    signal(SIGTTIN,SIG_DFL);
    signal(SIGTTOU,SIG_DFL);
    signal(SIGHUP,SIG_DFL);
    signal(SIGQUIT,SIG_DFL);
    
}

void execute_builtin(Cmd c, int nice, int nicevalue,int shift, char builtin){
    int infp=-1, outfp=-1, infp_bk=-1, outfp_bk=-1, errfp_bk=-1;

    if(builtin != 7){
        open_files_for_redirection(c,NULL, NULL, &infp, &outfp );
    }
    if(c->in != Tnil || c->out != Tnil){
        backup_fp(c,&infp_bk,&outfp_bk,&errfp_bk);
    }
    set_redirections(c,infp,outfp);
    switch(builtin){
        case 1: //echo
            break; 
        case 2: //cd       
            
            break;
        case 3: //pwd
            break;
        case 4: //setenv
            break;
        case 5: //unsetenv
            break;
        case 6: //logout
            break;
        case 7: //nice
            break;
        case 8: //where
            break;
        case 9: //fg
            break;
        case 10://bg
            break;
        case 11://jobs
            break;
        case 12://kill  
            break;
    }
    if(c->out != Tnil || c->in != Tnil){
        restore_fp(c,&infp_bk,&outfp_bk,&errfp_bk);
    }

}

int init_shell(){
    
    // If the shell is the foreground process do the following:
    int set_pg;

    ush_terminal=STDIN_FILENO;
    ush_interactive=isatty(ush_terminal);
    if(ush_interactive){
        
        disable_signals();        

        ush_pid = getpid();
        //Set the shell pid as the process group id        
        set_pg=setpgid(ush_pid,ush_pid);
        if(set_pg < 0){
            fputs("Unable to set shell process group id to its pid\n",stderr);
            return -1;
        }
        tcsetpgrp(ush_terminal,ush_pid);
        //Save terminal modes 
        tcgetattr(ush_terminal,&ush_modes); 
    }
    return 0;
}

void create_job(Pipe p, int nice, int nicevalue,int shift){

    Cmd c;
    int **pipes;
    int *in_pipe,*out_pipe
    int infp,outfp,infp_bk,outfp_bk,errfp_bk;
    int pipe_count=0,i;
    ush_process* temp,tail;
    ush_job* job;
    c=p->head;

    // Create all pipes
    while(c!=NULL){
        if(c->out==Tpipe || c->out==TpipeErr)
            pipe_count++;
        c=c->next;
    }
   
    if(pipe_count==0){
        i=is_built_in(c->args[0]);
        if(i>0){
            execute_builtin(c,nice,nicevalue,shift);
        }
        
    }
 
    job =(ush_job*)malloc(sizeof(ush_job));
    job->p=p;
    job->first=NULL;


    pipes=(int**)malloc(sizeof(int*)*(pipe_count+2));
    pipes[0]=NULL;
    pipes[pipe_count+1]=NULL;
    for(i=0;i<pipe_count;i++)
        pipes[i+1]=create_pipe();
    // Prepare process
    c=p->head;
    i=1;
    tail=job->first;
    while(c!=NULL){ 
        open_files_for_redirection(c,pipes[i-1],pipes[i],&infp, &outfp );
        temp = create_process(c,infp,outfp);
        if(tail!=NULL){
            tail->next=temp;
        }else{
            job->first=temp;
        }
        tail=temp;
        c=c->next;
        i=i+1;
    }
    tail=job->first;
    
    

}

ush_process* create_process(Cmd c, int infp, int outfp, int shift){

    ush_process proc = (ush_process*) malloc(sizeof(ush_process));
    proc->infp= infp;
    proc->outfp= outfp;
    proc->cmd=c;
    proc->args=(c->args)+shift;
    proc->builtin=is_built_in(c->args[shift]);
    proc->status = JOB_RUNNING;
    proc->shift=shift;
    proc->next=NULL;
    return proc;
}

void spawn_subprocess(ush_process* proc, int bg ,int nice, int nicevalue, pid_t pgid){
    
    pid_t pid;
    pid_t pid_new;
    int res;
    pid=fork();
    if(pid < 0){
        fputs("Error: Forking failed\n",stderr);
    }else if(pid == 0){
        pid_new=getpid();
        if(pgid == 0)
            pgid = pid_new;
        setpgid(pid_new,pgid);
        enable_signals();
        if(nice)
            nice_(nicevalue);
        set_redirections(proc->cmd,infp,outfp);
        if(is_built_in(proc->cmd->args[proc->shift])==0){
            res=execve(proc->cmd->args[proc->shift],(proc->cmd->args)+proc->shift);
            if(res<0){
                fputs("Error: Execve failed",stderr);
                exit(1);
            }
        }else{
            
        
        }
        
    }else{
        if(pgid == 0){
            pgid = pid;
        }
        setpgid(pid,pgid)
        proc->pid=pid;
        if(infp>2)close(infp);
        if(outfp>2)close(outfp);
        if(bg!=1)
            wait(&proc->status);
    }
} 
//void execute_in_subshell()
