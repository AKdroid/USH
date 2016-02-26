#ifndef REDIRECT_H
#define REDIRECT_H

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>

int open_file_for_read(char* fname);

int open_file_for_write(char* fname);

int open_file_for_append(char* fname);

int* create_pipe();

int redirect_stdin(int infp);

int redirect_stdout(int outfp);

int redirect_stdout_err(int outfp); 

void destroy_pipe(int* p);

#endif /* REDIRECT_H */


