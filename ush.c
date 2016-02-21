#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<fcntl.h>
#include<unistd.h>
#include"parse.h"

//Dissect Command

void dissect_command(Cmd c){
    int i;
    if(c){

        printf("%s : %s\n", c->exec==Tamp? "BG": "FG",c->args[0]);

        if(c->in == Tin){
            printf("Read input from %s\n",c->infile);
        }

        if(c->out != Tnil){
            switch(c->out){
                case Tout:
                    printf("Redirect output to %s\n",c->outfile);
                    break;
                case Tapp:
                    printf("Append the output to %s\n",c->outfile); 
                    break;
                case ToutErr:
                    printf("Redirect output and error to %s\n",c->outfile); 
                    break;
                case TappErr:
                    printf("Append output and error to %s\n",c->outfile); 
                    break;
                case Tpipe:
                    printf("Connect output to next process stdin\n");
                    break;
                case TpipeErr:
                    printf("Connect both output and errot to the next process's stdin\n");
                    break;
                default:
                    printf("Need more handling\n");
            }
        }
        printf("Number of Arguments: %d\n",c->nargs);
        for(i=0;i<c->nargs;i++){
            printf("Argument #%d : %s\n",i,c->args[i]);
        }
        if(strcmp(c->args[0],"exit")==0)
            exit(0);    
    }
}

void update_stdin(int* inptr, char* fname){
    *inptr = open(fname,O_RDONLY);
    dup2(*inptr,STDIN_FILENO);
}

void update_out_write(int* optr,char * fname,int err){
    *optr=open(fname,O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    dup2(*optr,STDOUT_FILENO);
     if(err)
        dup2(*optr,STDERR_FILENO);
}

void update_out_append(int* optr, char* fname, int err){
    
    *optr=open(fname,O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    dup2(*optr,STDOUT_FILENO);
    if(err)
        dup2(*optr,STDERR_FILENO);
}


int execute_fg(Cmd c){
    pid_t pid;
    int status,result,inptr,optr;
    if(c==NULL)
        return 0;
        
    pid=fork();
    if(pid < 0 )
        return -1;
    if(pid == 0){  
        if(c->in == Tin)
            update_stdin(&inptr,c->infile);
        switch(c->out){
            case Tout:
                update_out_write(&optr,c->outfile,0);
                break;
            case Tapp:
                update_out_append(&optr,c->outfile,0);
                break;
            case ToutErr:
                update_out_write(&optr,c->outfile,1);
                break;
            case TappErr:
                update_out_append(&optr,c->outfile,1);
                break;
        } 
        result = execvp(c->args[0],c->args);
        if(result < 0){
            printf("Execution Failed\n");
            exit(1);
        }
    }else {
        wait(&status);
    }
    return 0;
}


int main(){

    char* hostname = "arrao-ush";
    Pipe p;
    Cmd c;  
    char *argv[64];
    while(1){
        printf("%s %% ",hostname);
        p=parse();
        if(p!=NULL){
            c=p->head;
            dissect_command(c); 
            execute_fg(c);
        }
    } 

    return 0;
}
