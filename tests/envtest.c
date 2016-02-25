#include<stdio.h>
#include<stdlib.h>

extern char **environ;

int main(){

    int i=1;
    char *s;
    s=*environ;
/*    for(;s;i++){
        printf("%s\n",s);
        s=*(environ+i);
    }
  */  
    putenv("AKHIL=GENIUS");    
    putenv("AKHIL");
    s=*environ;
    for(i=1;s;i++){
        printf("%s\n",s);
        s=*(environ+i);
    }

    printf("AKHIL:%s\n",getenv("AKHIL"));
    

    return 0;
}
