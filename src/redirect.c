#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>
#include"parse.h"
#include"redirect.h"

/*
    Author: Akhil Raghavendra Rao (arrao@ncsu.edu)
    
    Provides APIs for redirections and backup/restores of STDIN/STDOUT;
*/


int open_file_for_read(char* fname){
    /*
        Open the file as READONLY. Used for input redirection.
    */
    int fp;
    fp = open(fname,O_RDONLY);
    return fp;
}

int open_file_for_write(char* fname){
    /*
        Open the file for WRITE with TRUNCATION. Used for non-append writes.
    */
    int fp;
    fp = open(fname,O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    return fp;
}

int open_file_for_append(char* fname){
    /*  
     * Open the file for appending. Used for append writes redirections.
     */
    int fp;
    fp = open(fname,O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    return fp;
}

int* create_pipe(){
    /*
     * returns pointers to a 2 iterm array of pipes. 
     * Should be later destroyed with destroy_pipe 
     */
    int* pp = (int*)malloc(2*sizeof(int));
    pipe(pp);
    return pp;
}

void destroy_pipe(int *p){
    /*
     * Destroys the pipe created from create_pipe
     */
    free(p);
}

int redirect_stdin(int infp){
    /*
     * Redirects the STDIN to file at location infp (file pointer)
     */
    dup2(infp,STDIN_FILENO);
}

int redirect_stdout(int outfp){
    /*
     * Redirects the STDOUT to file at location outfp (file pointer)
     */
    dup2(outfp, STDOUT_FILENO);
}

int redirect_stdout_err(int outfp){
    /*
     * Redirects the STDOUT and STDERR to file at location outfp (file pointer)
     */
    dup2(outfp,STDOUT_FILENO);
    dup2(outfp,STDERR_FILENO);
}

void backup_fp(Cmd c, int* infp_bk, int* outfp_bk, int* errfp_bk){
    /*
     * Backs up all STD file pointers i.e. STDIN, STDOUT abd STDERR depending on the cmd requirement.
     * If backup is not required -1 is stored.
     */
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

void open_files_for_redirection(Cmd c,int* in_pipe, int* out_pipe, int* infp, int* outfp ){
    /*
     * Depending on the cmd, sets the value of infp and outfp 
     * in_pipe and out_pipe are used for PIPE scenarios
     * If redirection is not required respective value is set to -1.
     */
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

void restore_fp(int infp_bk, int outfp_bk, int errfp_bk){
    /*
     * If the given pointers are non negative, respective STD<X> are restored.
     */
    if(infp_bk>=0)
        dup2(infp_bk,STDIN_FILENO);
    if(outfp_bk>=0)
        dup2(outfp_bk,STDOUT_FILENO);
    if(errfp_bk>=0)
        dup2(errfp_bk,STDERR_FILENO);
}

void set_redirections(Cmd c, int infp, int outfp){
    /*
     * Performs the redirections as required by cmd c to the files 
     * pointed by infp and outfp. 
     * A value of -1 of infp or outfp indicates no action required    
     */
    if(c->in != Tnil && infp >=0)
        redirect_stdin(infp);
    if(outfp>=0 && (c->out== Tout || c->out == Tapp || c->out == Tpipe))
        redirect_stdout(outfp);
    if(outfp>=0 && (c->out== ToutErr || c->out == TappErr || c->out == TpipeErr))
        redirect_stdout_err(outfp);
}

