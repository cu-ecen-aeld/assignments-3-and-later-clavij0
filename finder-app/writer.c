#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

int main(int argc, char *argv[]) {
    //printf("argc=%d\n",argc);
    //syslog(LOG_ERR,"Invalid Numner of arguments: %d", argc);
    openlog("write.c", LOG_PID|LOG_CONS, LOG_USER);
    syslog(LOG_INFO, "Enter to Assginment2 ");
//    closelog();
     if(argc<3){
        syslog(LOG_ERR,"Invalid Number of arguments, please specify the filename and string: %d", argc);
        //printf("ERROR 1\n");
        exit(1);
    }else if (argc>3)
    {
        syslog(LOG_ERR,"Invalid Number of arguments, MAX arguments 2 you have introduced: %d", argc);
        exit(1);
    }else{
        int i;
        FILE * fptr;
        char *filename;
        

        filename = argv[1];
        fptr = fopen(filename, "w"); 
        if (fptr != NULL){
            syslog(LOG_INFO, "Correctly entered arguments");
            syslog(LOG_DEBUG,"Writing %s to %s", argv[2], argv[1]);
            char *str = argv[2];
            fputs(argv[2],fptr);
            fputs("\n",fptr);
            fclose(fptr);

        }else{
            syslog(LOG_ERR,"Missing Filename and Text");        
        }

    }
       
    return 0;
}