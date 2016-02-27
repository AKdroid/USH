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

ush_job* job_head = NULL;

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

void execute_builtin(Cmd c, int nice, int nicevalue,int shift, char builtin, int open_, int in_, int ou_){
    int infp=-1, outfp=-1, infp_bk=-1, outfp_bk=-1, errfp_bk=-1;
    char* name,*value;

    if(builtin != 7 && open_==1){
        open_files_for_redirection(c,NULL, NULL, &infp, &outfp );
    }
    if(open_ == 0){
        infp=in_;
        outfp=ou_;
    }
    if(c->in != Tnil || c->out != Tnil){
        backup_fp(c,&infp_bk,&outfp_bk,&errfp_bk);
    }
    set_redirections(c,infp,outfp);
    switch(builtin){
        case 1: //echo
            echo((c->args)+1+shift);
            break; 
        case 2: //cd       
            if(c->nargs==1+shift)
                cd(NULL);
            else
                cd(c->args[1+shift]);
            break;
        case 3: //pwd
            pwd();
            break;
        case 4: //setenv
            name=NULL;
            value=NULL;
            if(c->nargs>1+shift)
                name=c->args[1+shift];
            if(c->nargs>2+shift)
                value=c->args[2+shift];
            setenv_(name,value);
            break;
        case 5: //unsetenv
            if(c->nargs>1+shift)
                fputs("Error: unsetenv takes one argument",stderr);
            else
                unsetenv_(c->args[1+shift]);
            break;
        case 6: //logout
            logout();
            break;
        case 7: //nice
            
            break;
        case 8: //where
            if(c->nargs>1+shift)
                where(c->args[1+shift]);
            else
                fputs("Error: where requires an argument\n",stderr);
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
        restore_fp(infp_bk,outfp_bk,errfp_bk);
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

int create_job(Pipe p, int nice, int nicevalue,int shift){

    Cmd c;
    int **pipes;
    int infp,outfp,infp_bk,outfp_bk,errfp_bk;
    int pipe_count=0,i,bg;
    char ch;
    ush_process* temp,*tail;
    ush_job* job,*tempj;
    c=p->head;
    if(c==NULL)
        return 1;
    if(strcmp(c->args[0],"end")==0){
        return 0;
    }
    printf("%s\n",c->args[0]);
    // Create all pipes
    while(c!=NULL){
        if(c->out==Tpipe || c->out==TpipeErr)
            pipe_count++;
        c=c->next;
    }
    if(pipe_count==0){
        c=p->head;
        ch=is_built_in(c->args[0]);
        if(ch>0){
            execute_builtin(c,nice,nicevalue,shift,ch,1,0,0);
            return 1;
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
        temp = create_process(c,infp,outfp,shift);
        if(tail!=NULL){
            tail->next=temp;
        }else{
            job->first=temp;
        }
        tail=temp;
        bg=c->exec==Tamp;
        c=c->next;
        i=i+1;
    }
    job->pgid=-1;
    
    tempj=job_head;
    if(tempj==NULL)
        job_head=job;
    else{
        while(tempj->next != NULL){
            tempj=tempj->next;
        }
        tempj->next=job;
    }
    tail=job->first;
    while(tail!=NULL){
        if(tail->next==NULL){
            if(tail->builtin>0){
                execute_builtin(tail->cmd,nice,nicevalue,tail->shift,tail->builtin,0,tail->infp,tail->outfp);
            }else{
                spawn_subprocess(job,tail,bg,nice,nicevalue,job->pgid);
            }
        }else{
             spawn_subprocess(job,tail,bg,nice,nicevalue,job->pgid);
        }
        
        tail=tail->next;
    } 
    return 1;    
}

ush_process* create_process(Cmd c, int infp, int outfp, int shift){

    ush_process* proc = (ush_process*) malloc(sizeof(ush_process));
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

void spawn_subprocess(ush_job* job,ush_process* proc, int bg ,int nice, int nicevalue, pid_t pgid){
    
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
        if(bg!=1)
            tcsetpgrp(ush_terminal,pgid);
        if(nice)
            nice_(nicevalue);
        set_redirections(proc->cmd,proc->infp,proc->outfp);
        if(is_built_in(proc->cmd->args[proc->shift])==0){
            res=execvp(proc->cmd->args[proc->shift],(proc->cmd->args)+proc->shift);
            if(res<0){
                fputs("Error: Execve failed",stderr);
                exit(1);
            }
        }else{
            proc->cmd->in=Tnil;
            proc->cmd->out=Tnil;
            execute_builtin(proc->cmd,nice,nicevalue,proc->shift,proc->builtin,0,-1,-1);
            exit(0);    
        }
        
    }else{
        if(pgid == -1){
            pgid = pid;
            job->pgid=pgid;
        }
        setpgid(pid,pgid);
        proc->pid=pid;
        if(proc->infp>2)close(proc->infp);
        if(proc->outfp>2)close(proc->outfp);
        if(bg!=1){
            tcsetpgrp(ush_terminal,pgid);
            wait(&proc->status);
            tcsetpgrp(ush_terminal,ush_pid);
            tcgetattr(ush_terminal,&job->modes);
            tcsetattr(ush_terminal,TCSADRAIN,&ush_modes);
        }
    }
} 
//void execute_in_subshell()
