#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>
#include"parse.h"
#include"redirect.h"

int open_file_for_read(char* fname){
   
    int fp;
    fp = open(fname,O_RDONLY);
    return fp;
}

int open_file_for_write(char* fname){
    int fp;
    fp = open(fname,O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    return fp;
}

int open_file_for_append(char* fname){
    int fp;
    fp = open(fname,O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    return fp;
}

int* create_pipe(){
    int* pp = (int*)malloc(2*sizeof(int));
    pipe(pp);
    return pp;
}

void destroy_pipe(int *p){
    free(p);
}

int redirect_stdin(int infp){
    dup2(infp,STDIN_FILENO);
}

int redirect_stdout(int outfp){
    printf("Called redirect stdout\n");
    dup2(outfp, STDOUT_FILENO);
}

int redirect_stdout_err(int outfp){
    dup2(outfp,STDOUT_FILENO);
    dup2(outfp,STDERR_FILENO);
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

