#ifndef REDIRECT_H
#define REDIRECT_H

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include"parse.h"

//Get a pipe 
int* create_pipe();

//Destroy a retrieved pipe
void destroy_pipe(int* p);

//Backup file pointers
void backup_fp(Cmd c, int* infp_bk, int* outfp_bk, int* errfp_bk);

//Restore the backed up file pointers
void restore_fp(int infp_bk, int outfp_bk, int errfp_bk);

//Set the redirections for the command c
void set_redirections(Cmd c, int infp, int outfp);

//Set the infp and outfp values for command c
void open_files_for_redirection(Cmd c,int* in_pipe, int* out_pipe, int* infp, int* outfp );

#endif /* REDIRECT_H */


