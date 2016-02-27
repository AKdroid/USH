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
    struct termios modes;
    struct ush_process* first;
};

typedef struct ush_process ush_process;
typedef struct ush_job ush_job;

int init_shell();

int create_job(Pipe p, int nice, int nicevalue,int shift);

void fg(int id);

void bg(int id);

void kill_(int id);

void jobs();

ush_process* create_process(Cmd c, int infp, int outfp, int shift);

void spawn_subprocess(ush_job* job,ush_process* proc, int bg ,int nice, int nicevalue, pid_t pgid);

void execute_builtin(Cmd c, int nice, int nicevalue,int shift, char builtin, int open_, int in_, int ou_);

#endif
