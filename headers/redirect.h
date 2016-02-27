#ifndef REDIRECT_H
#define REDIRECT_H

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include"parse.h"

int open_file_for_read(char* fname);

int open_file_for_write(char* fname);

int open_file_for_append(char* fname);

int* create_pipe();

int redirect_stdin(int infp);

int redirect_stdout(int outfp);

int redirect_stdout_err(int outfp); 

void destroy_pipe(int* p);

void backup_fp(Cmd c, int* infp_bk, int* outfp_bk, int* errfp_bk);

void restore_fp(int infp_bk, int outfp_bk, int errfp_bk)

void set_redirections(Cmd c, int infp, int outfp);

void open_files_for_redirection(Cmd c,int* in_pipe, int* out_pipe, int* infp, int* outfp );

#endif /* REDIRECT_H */


