#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<signal.h>
#include<termios.h>
#include<sys/wait.h>
#include"redirect.h"
#include"jobs.h"

/*
    Author: Akhil Raghavendra Rao (arrao@ncsu.edu)
    Job Control Implementation 
    Job builtin Implementation
    
    
*/


int ush_pid; //USH Process Group
int ush_terminal; //USH File Pointer
struct termios ush_modes; //USH Terminal Modes Storage
int ush_interactive; //USH Interactive Mode or Not

int sigterm_count=0; //Signal termination count

int stop_counter = STBASE; //Counter to set event_index for stopped jobs
int bg_counter = BGBASE;   //Counter to set event_index for background jobs

ush_job* job_head = NULL;  //Head of the job list
pid_t most_recent = 0; //Stores the pgid of current job

void sigterm_handler(int num){
    /*
     * Handles the SIGTERM signal
     * Send SIGTERM all the existing jobs
     * Performs cleanup  
     */     
    ush_job* job;
    if(num == SIGTERM && sigterm_count==0){
        sigterm_count=1;
        
        for(job=job_head;job!=NULL;job=job->next){
            kill(-job->pgid,SIGTERM);
            kill(-job->pgid,SIGCONT);
            job->status=JOB_KILLED;
        }
        update_job_status();
        fflush(stdout);
        signal(SIGTERM,SIG_DFL);
        raise(SIGTERM);
    }
}

void job_cleanup(){
    ush_job* job;
    for(job=job_head;job!=NULL;job=job->next){
            kill(-job->pgid,SIGTERM);
            kill(-job->pgid,SIGCONT);
            job->status=JOB_KILLED;
        }
        update_job_status();
        fflush(stdout);

}


long int is_numeric(char* str, int* valid){
    /*
     * returns integer value of str and sets valid to 1 if str is a perfectly parsable 
     */
    char *eptr;
    long int ans=0;
    ans=strtol(str,&eptr,10);
    if(*eptr!='\0')
        *valid=0;
    else
        *valid=1;
    return ans;
}

void disable_signals(){
    /*
     *  Disable signals 
     */   
    signal(SIGINT,SIG_IGN);
    signal(SIGTSTP,SIG_IGN);
    signal(SIGTTIN,SIG_IGN);
    signal(SIGTTOU,SIG_IGN);
    signal(SIGHUP,SIG_IGN);
    signal(SIGQUIT,SIG_IGN);
}

void enable_signals(){
    /*
     * Enable Signals to their default handler
     */
    signal(SIGINT,SIG_DFL);
    signal(SIGTSTP,SIG_DFL);
    signal(SIGTTIN,SIG_DFL);
    signal(SIGTTOU,SIG_DFL);
    signal(SIGHUP,SIG_DFL);
    signal(SIGQUIT,SIG_DFL);
    signal(SIGTERM,SIG_DFL); 
}

void print_job(ush_job* j, int index){
    /*
     * Prints a job in a formatted way
     */ 
    int i;
    Cmd c;
    char* status;
    switch(j->status){
        case JOB_RUNNING:
            status = "Running";
            break;
        case JOB_STOPPED:
            status = "Stopped";
            break;
        case JOB_COMPLETED:
            status = "Done";
            break;
        case JOB_KILLED:
            status="Killed";
            break;
    }
    printf("[%d]%c\t%s \t",index,most_recent==j->pgid?'+':'-',status);
    c=j->p->head;
    
    while(c!=NULL){
        //for(i=0;i<c->nargs;i++){
        printf("%s ",c->args[0]);
        if(c->next!=NULL)
            printf("| ");
        c=c->next;
    }
    printf("\n");
}

void execute_nice_process(Cmd c, int nice, int nicevalue, int shift){
    /*
     *  If parameter of nice is builtin, execute in the same process
     *  If calling process is executed in same process as main shell,  create job
     *  If calling process is executed in a subprocess, spawn a new process and wait for its completion
     */ 
    pid_t pid;
    int res,status,open_=1;
    char ch;
    Pipe p;
    ch = is_built_in(c->args[shift]);

    if(ch == 0){ 
        if (c->out != Tpipe && c->out != TpipeErr){
            p=(Pipe) malloc(sizeof(struct pipe_t));
            p->head=c;
            p->next=NULL;
            create_job(p,nice,nicevalue,shift);
            return;  
        }
    pid  = fork();
    if(pid<0){
        return;
    } else if(pid == 0){
        nice_(nicevalue);
        if(execvp(c->args[shift],c->args+shift) < 0){
            printf("%S: Command not found.\n",c->args[shift]);
            exit(1);
        }
    } else {
        waitpid(pid,NULL,0);    
    }
    }
    else{
        if (c->out == Tpipe && c->out == TpipeErr){
            open_=0;
        }
        execute_builtin(c,nice,nicevalue,shift,ch,open_,0,0);
    }
}

ush_job* get_job_by_index(int index){
    /*
     * Returns job at index "index"
     */
    int i=1;
    ush_job* temp=job_head;
    while(temp!=NULL){
        if(i==index)
            break;
        temp=temp->next;
        i=i+1;
    }
    return temp;
}

ush_job* get_job_by_pgid(pid_t pgid){
    /*
     * Returns job with process group "pgid"
     */ 
    ush_job* temp = job_head;
    while(temp!=NULL){
        if(temp->pgid == pgid)
            break;
        temp=temp->next;
    }
    return temp;
}

int get_job_index(pid_t pgid){
    /*
     * Returns the index of the jovb with process group ID pgid
     */
    int i=1;
    ush_job* temp = job_head;
    while(temp!=NULL){
        if(temp->pgid == pgid)
            return i;
        temp=temp->next;
        i=i+1;
    }
    return 0;

}

void destroy_job(ush_job* job){
    /*
     * Performs cleanup of job. Deletes it and internal pipes
     */ 
    ush_process* p,*temp;
    Pipe pp;
    int shift;
    //jobs();
    p=job->first;
    shift=p->shift;
    while(p!=NULL){ 
        temp=p;
        p=p->next;
        free(temp);
    }
    pp=job->p;
    if(shift!=0)
        free(pp);
    else
        freePipe(pp);
    free(job);
}

void unlink_job(pid_t pgid){
    /* 
     *  Unlinks the job with process group pgid and destroys it.
     */
    ush_job* temp=job_head,* prev = NULL;
    while(temp!=NULL){
        if(temp->pgid == pgid){
            break;
        }
        prev=temp;
        temp=temp->next;
    }
    
    if(temp!=NULL){
        if(job_head == temp)
            job_head=job_head->next;
        else
            prev->next=temp->next;
        temp->next=NULL;
        destroy_job(temp);
    }
}

int wait_for_job(ush_job* job,int fg){
    /*
     * Wait for a job to stop/finish/killed
     * Updates the status of the job when a change os state is detected using waitpid
     * if fg is set to 1, process waits for an event change else unblocking waitpid call
     */
    int options;
    int result;
    ush_process *proc;
    int stopped;
    if(fg == 1){
        options = WUNTRACED;
    }else{
        options = WNOHANG | WUNTRACED;
    }
    proc = job->first;
    stopped=0;
    if(job->status == JOB_STOPPED)
        return 0;
    while(proc!= NULL){
        if(proc->pid<=0){
            proc=proc->next;
            continue;
        }
        result = waitpid(proc->pid,&job->return_value,options);
        if(result == proc->pid){
            if(WIFSTOPPED(job->return_value)){
                //printf("[%d]+ Stopped! %d\n",-1, job->pgid);
                job->status=JOB_STOPPED;
                tcgetattr(ush_terminal,&job->modes);
                stopped = 1;
                job->event_index = stop_counter++;
                break;
            }
        } else if(result == 0){
            stopped = 2;
            break;
        }
        proc=proc->next;
    }
    if(stopped==0){
        if(WTERMSIG(job->return_value) == SIGTERM | WTERMSIG(job->return_value) == SIGKILL){
            job->status = JOB_KILLED;
            job->event_index = -1;
        }
        else{
            job->status=JOB_COMPLETED;
            job->event_index = 0;
        }
    }
    if(fg == 1){
        tcsetpgrp(ush_terminal,ush_pid);
        tcsetattr(ush_terminal,TCSADRAIN,&ush_modes);        
    }
    return stopped;
}

void update_job_status(){

    /*
     *  Check the job status for background and stopped jobs
     *  Should be executed before parsing the command line input
     */
    ush_job* job,*prev_job,*temp;
    ush_process* proc;
    int index,result,stopped,maxindex;
    prev_job=NULL;
    job = job_head;
    index = 1;
    most_recent = 0;
    maxindex=0;
    while(job!=NULL){
        stopped = wait_for_job(job,0);
        if(job->status == JOB_COMPLETED || job->status == JOB_KILLED){
            temp = job;
            print_job(job,index);
            if(prev_job==NULL)
                job_head = job->next;
            else{
                prev_job->next = job->next;
            }
        }
        else{
            if(stopped == 1){
                print_job(job,index);
            }
            if(maxindex < job->event_index){
                maxindex = job->event_index;
                most_recent = job->pgid;
            }
            prev_job=job;
            temp=NULL;
        }
        job=job->next;
        if(temp!=NULL)
            destroy_job(temp);
        index = index + 1;
    }
}

void execute_builtin(Cmd c, int nice, int nicevalue,int shift, char builtin, int open_, int in_, int ou_){
    /*
     * Executes a builtin command
     * nice = 1 :- set the nice value to nicevlaue
     * shift = index at which target command is found
     * builtin = code of the builtin to be executed
     * open_ = 0 -> do not open any files but use in_ and ou_, 
     * open_ = 1 -> open files if required for redirection, 
     * open_ = 2 -> do not open or backup any files for redirection  
     */
    int infp=-1, outfp=-1, infp_bk=-1, outfp_bk=-1, errfp_bk=-1,index,valid,nv;
    char* name,*value;
    if(builtin != 7 && open_==1){
        open_files_for_redirection(c,NULL, NULL, &infp, &outfp );
    }
    if(open_ == 0){
        infp=in_;
        outfp=ou_;
    }
    if(open_!= -1 &&(c->in != Tnil || c->out != Tnil)){
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
            job_cleanup();
            logout();
            break;
        case 7: //nice
            if(c->nargs<=shift+1){
                //fputs("Error: nice requires at least 1 argument\n",stderr);
            }else{
                nv=(int)is_numeric(c->args[shift+1],&valid);
                if(valid){
                    if(c->nargs>=shift+3)
                        execute_nice_process(c,1,nv,shift+2);
                }else{
                    nv=4;
                    execute_nice_process(c,1,nv,shift+1);
                }
            } 
            break;
        case 8: //where
            if(c->nargs>1+shift)
                where(c->args[1+shift]);
            else
                fputs("Error: where requires an argument\n",stderr);
            break;
        case 9: //fg
            if(c->nargs>1){
                if(c->args[1][0]=='%')
                    index=is_numeric((c->args[1]+1),&valid);
                else
                    index=is_numeric(c->args[1],&valid);
                if(valid)
                    fg(index);
            }
            else
            fg(-1);
            break;
        case 10://bg
            if(c->nargs>1){
                if(c->args[1][0]=='%')
                    index=is_numeric((c->args[1]+1),&valid);
                else
                    index=is_numeric(c->args[1],&valid);
                if(valid)
                    bg(index);
            } else 
                bg(-1);
            break;
        case 11://jobs
            jobs();
            break;
        case 12://kill  
            if(c->nargs>1){
                if(c->args[1][0]=='%')
                    index=is_numeric((c->args[1]+1),&valid);
                else
                    index=is_numeric(c->args[1],&valid);
                if(valid)
                    kill_index(index);
            }else 
            kill_index(-1);
            break;
    }
    if(open_!= -1 &&(c->in != Tnil || c->out != Tnil)){
        restore_fp(infp_bk,outfp_bk,errfp_bk);
    }

}

int init_shell(){
    /*
     * Initialise the shell.
     * Disable keyboard interrupts
     * Setup the signal handler
     * MAke it foreground
     */
     
    // If the shell is the foreground process do the following:
    int set_pg;

    ush_terminal=STDIN_FILENO;
    ush_interactive=isatty(ush_terminal);
    if(ush_interactive){
        
        disable_signals();        
        
        signal(SIGTERM,sigterm_handler);

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

int create_job(Pipe p, int nice, int nicevalue, int shift){
    /*
     * Creates a job if not builtin
     * Creates the pipes
     * Initializes the job and its processes
     * Add the job to the linked list 
     * Spawn the subprocesses if required
     * If foreground wait for job completion/stop. If job completed/killed, unlink the job 
     */
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
    if(c->in == Tin){
        if(access(c->infile,R_OK)<0){
            printf("%s: No such file or directory\n",c->infile);
            return 1;
        }
    }
    // Create all pipes
    while(c!=NULL){
        if(c->out==Tpipe || c->out==TpipeErr)
            pipe_count++;
        c=c->next;
    }
    if(pipe_count==0){
        c=p->head;
        ch=is_built_in(c->args[shift]);
        if(ch>0){
            execute_builtin(c,nice,nicevalue,shift,ch,1,0,0);
            if(shift!=0)
                free(p);
            else
                freePipe(p);
            return 1;
        }
        
    }
 
    job =(ush_job*)malloc(sizeof(ush_job));
    job->p=p;
    job->first=NULL;
    job->return_value=-1;
    job->event_index = 0;
    job->next=NULL;
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
    job->status=JOB_RUNNING;
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

    for(i=1;i<=pipe_count;i++){
        destroy_pipe(pipes[i]);
    }   
    free(pipes);
    if(bg != 1){
        wait_for_job(job,1);
        if(job->status == JOB_COMPLETED)
            unlink_job(job->pgid);
    }
    return 1;    
}

ush_process* create_process(Cmd c, int infp, int outfp, int shift){
    /*
     *  Initialize a ush_process 
     */
    ush_process* proc = (ush_process*) malloc(sizeof(ush_process));
    proc->infp= infp;
    proc->outfp= outfp;
    proc->cmd=c;
    proc->builtin=is_built_in(c->args[shift]);
    proc->shift=shift;
    proc->next=NULL;
    return proc;
}

void spawn_subprocess(ush_job* job,ush_process* proc, int bg ,int nice, int nicevalue, pid_t pgid){
    /*
     *   Spawns a subprocess. Sets the pgid to pgid and if -1, sets the pgid to pid. 
     *   Makes the pgid foreground if bg != 1  
     */
    pid_t pid;
    pid_t pid_new;
    int res;
    ush_process *x;
    pid=fork();
    if(pid < 0){
        fputs("Error: Forking failed\n",stderr);
    }else if(pid == 0){
        pid_new=getpid();
        if(pgid == 0)
            pgid = pid_new;
        setpgid(pid_new,pgid);
        enable_signals();
        if(bg!=1){
            tcsetpgrp(ush_terminal,pgid);
        }
        if(nice)
            nice_(nicevalue);
        set_redirections(proc->cmd,proc->infp,proc->outfp);
        if(is_built_in(proc->cmd->args[proc->shift])==0){
            res=execvp(proc->cmd->args[proc->shift],(proc->cmd->args)+proc->shift);
            if(res<0){
                printf("%s:  Command not found.\n",proc->cmd->args[0]);
                exit(1);
            }
        }else{
            if(proc->builtin!=7){
            proc->cmd->in=Tnil;
            proc->cmd->out=Tnil;
            res=0;
            }else{
                res=-1;
            }
            execute_builtin(proc->cmd,nice,nicevalue,proc->shift,proc->builtin,res,-1,-1);
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
        }
        else{
            job->event_index=bg_counter++;
            printf("[%d] %d\n",get_job_index(pgid),pgid);            
            usleep(100000);
            
        }
    }
} 

void fg(int index){
    /*
     * Puts the job at index to foreground
     * if index = -1, current job is put to foreground
     */ 
    ush_job* job;
    if(index != -1)
        job  = get_job_by_index(index);
    else {
        if (most_recent > 0)
             job = get_job_by_pgid(most_recent);
        else
            return;
    }
    if(job== NULL){
        fputs("Job of the given index does not exist\n",stderr);
        return;
    }
    
    tcsetpgrp(ush_terminal,job->pgid);
    //tcsetattr(ush_terminal,TCSADRAIN, &(job->modes));
    if(job->status == JOB_STOPPED){
        job->status=JOB_RUNNING;
        tcsetattr(ush_terminal,TCSADRAIN, &(job->modes));
        kill(- job->pgid, SIGCONT);
    }
        
    job->event_index = 0;
    wait_for_job(job,1);
    if(job->status == JOB_COMPLETED)
        unlink_job(job->pgid);
}

void bg(int index){
    /*
     * Puts the job at index to background
     * if index = -1, current job is put to background
     */
    ush_job* job;
    if(index != -1)
        job  = get_job_by_index(index);
    else {
        if (most_recent > 0)
             job = get_job_by_pgid(most_recent);
        else
            return;    
    }    
    if(job== NULL){
        fputs("Job of the given index does not exist\n",stderr);
        return;
    }
    if(job->status == JOB_STOPPED){
        job->status=JOB_RUNNING;
        kill(-job->pgid, SIGCONT);
    } else {
        fputs("Job already in running state\m",stderr);
    }
    job->event_index = bg_counter++;
}

void kill_index(int index){
    /*
     * Sends a SIGTERM to job at index "index"
     * if index = -1, current job is terminated
     */
    ush_job* job;
    if(index != -1)
        job  = get_job_by_index(index);
    else {
        if (most_recent > 0)
             job = get_job_by_pgid(most_recent);
        else
            return;
    }

    if(job== NULL){
        fputs("Job of the given index does not exist\n",stderr);
        return;
    }
    job->status = -1;
    kill(-job->pgid,SIGTERM);
    kill(-job->pgid,SIGCONT);
    job->status=JOB_KILLED;
}

void jobs(){
    /*
     * Shows the list of active jobs
     */
    ush_job* job;
    int index=1;
    job = job_head;
    while(job!=NULL){
        print_job(job,index);
        job=job->next;
        index = index + 1;
    }

}
