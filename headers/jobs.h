#ifndef JOBS_H
#define JOBS_H

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<signal.h>
#include<termios.h>
#include"parse.h"


typedef enum { JOB_FG, JOB_BG, JOB_STOPPED, JOB_COMPLETED, JOB_RUNNING} JobStatus ;

struct ush_process{
    char** args;
    Cmd cmd;
    pid_t pid;    
    int returnVal;
    JobStatus status;
    char builtin;
    int infp,outfp,shift;
    struct ush_process* next;
};


struct ush_job{
    
    struct ush_job *next;
    JobStatus status;
    Pipe p;
    pid_t pgid;
    char builtin;
    struct termios modes;
    struct ush_process* first;
};

typedef struct ush_process ush_process;
typedef struct ush_job ush_job;

int init_shell();

void create_job(Pipe p);

void fg(int id);

void bg(int id);

void kill_(int id);

void jobs();


#endif
