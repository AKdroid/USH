#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<fcntl.h>
#include"builtins.h"

extern char** environ;
int environ_size=0;

void init_environment(){
    int i=1;
    char*x=*environ;
    environ_size=0;
    for(i=1;x;i++){
        environ_size+=1;
        x=*(environ+i);
    }
    
}

void echo(char ** words){
    char *s=*words;
    int i=1;
    for(i=1;s;i++){
        printf("%s ",s);
        s=*(words+i);
    }
    printf("\n");
}
void pwd(){
    char path[512];
    if( getcwd(path,512) != 0)
        printf("%s\n",path);
    else
        printf("Error in getting current directory\n");
}

//Changes the directory to path if it exists and has permissions
int cd(char* path){
    
    int result;
    if(path == NULL )
        path = getenv("HOME");
    result = chdir(path);
    if(result != 0){
        printf("Error Occured while changing the directory\n");
        return -1;
    }
    return 0;
}

//Lists all the environment variables if both var and value is null, sets var to null string if value is null, 
void setenv_(char* var, char* value){
    char* x="";
    char *y;
    int i;
    if(var == NULL){
        x=*environ;
        for(i=1;x;i++){
            printf("%s\n",x);
            x=*(environ+i);
        }
    }else{
        if(value!=NULL){
            y=(char*)malloc(strlen(var)+strlen(value)+2);
            x=var;
            i=0;
            while(*x)
            {       
                y[i++]=*x;
                x++;
            }
            y[i++]='=';
            x=value;
            while(*x)
            {
                y[i++]=*x;
                x++;
            }
            y[i]=0;
        }else{
            y=(char*)malloc(strlen(var)+2);
            x=var;
            i=0;
            while(*x)
            {
                y[i++]=*x;
                x++;
            }
            y[i++]='=';
            y[i]=0;
        }
        putenv(y);
    }
}


//Unset a variable from the environment
void unsetenv_(char* name){
    int l,i,f,cnt=0;
    char **x,**y,*z;
    f=0;
    l=strlen(name);
    x=environ;
    while(*x!=NULL){
        if(f==0)
            cnt+=1;
        if(f==0&&strncmp(name,*x,l)==0&&(*x)[l]=='='){
            z=*x;
            f=1;
        }
        if(f==1){
           *x=*(x+1); 
        }
        x++;
    }
    if(z!=NULL){
        if(cnt>environ_size)
            free(z);
        else
            environ_size-=1;
    }
}

//Exit the shell
void logout(){
    exit(0);
}

//Set the nice value of the command
void nice_(int x){
    nice(x);    
}

int is_built_in(char* cmd){
    char* builtins[13];
    char*x;
    int i;
    builtins[0]="echo";
    builtins[1]="cd";
    builtins[2]="pwd";
    builtins[3]="setenv";
    builtins[4]="unsetenv";
    builtins[5]= "logout";
    builtins[6]="nice";
    builtins[7]="where";
    builtins[8]="fg";
    builtins[9]="bg";
    builtins[10]="jobs";
    builtins[11]="kill";
    builtins[12]=NULL;
    x=*builtins;
    for(i=1;x;i++){
        if(strcmp(x,cmd)==0){
            return i;
        }
        x=*(builtins+i);
    }
    return 0;
}

void where(char* cmd){

    int builtin,i;
    char* PATH,*pptr;    
    char abspath[2000];
    builtin = is_built_in(cmd);
    if (builtin)
        printf("%s: shell built-in command.\n",cmd);
    PATH=getenv("PATH");
    //printf("%s\n",PATH);
    pptr=PATH;
    while(*pptr){
        i=0;
        while(*pptr && *pptr !=':'){
            abspath[i++]=*pptr;
            pptr++;
        }
        if(*pptr==':')
            pptr++;
        if(i>0 && abspath[i-1]!='/')
            abspath[i++]='/';
        abspath[i++]=0;
        strcat(abspath,cmd);
        if(access(abspath,X_OK)==0){
            printf("%s\n",abspath);
        }
    }    
}

