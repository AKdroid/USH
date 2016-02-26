#ifndef BUILT_IN_H
#define BUILT_IN_H

#include<stdio.h>
#include<stdlib.h>
/*

    Provides declarations for ush builtins except job builtins

*/

typedef enum {ECHO, PWD, CD, SETENV, UNSETENV, LOGOUT, NICE ,CMD} builtin_type;

void init_environment();
// Print out the words on the stdout, filename is NULL -> stdout else, output redireced to filename
//void echo(char** words, char* filename, int append);

void echo(char **words);

// Get current working directory
void pwd();

//Changes the directory to path if it exists and has permissions
int cd(char* path);

//Lists all the environment variables if both var and value is null, sets var to null string if value is null, 
void setenv_(char* var, char* value);

//Unset a variable from the environment
void unsetenv_(char* name);

//Exit the shell
void logout();

//Set the nice value of the command
void nice_(int x);

//where goes through the PATH variables listing all instances of an executable
void where(char* cmd);

//Check whether cmd is built in
int is_built_in(char *cmd);
#endif /* BUILT_IN_H */


