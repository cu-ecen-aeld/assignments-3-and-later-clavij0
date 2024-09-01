/*
**Base on https://beej.us/guide/bgnet/html/#lowlevel
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <syslog.h>
#include <fcntl.h>
#include <stdbool.h>

#define PORT "9002"  // the port users will be connecting to

#define BACKLOG 10   // how many pending connections queue will hold

#define WEB "www.sirclavt.shop"

#define MAXBUFLEN 1024

#define FILE_NAME "/var/tmp/aesdsocketdata"

int sockett;
FILE * fptr;

/*
Step
0- addrinfo define sockaddr_in or sockaddr 0.1-sockaddr_in
1- getaddrinfo()
2- socket() Get File descriptor -> int socket(int domain, int type, int protocol); 
3- bind() Port, when need to connect on specific port -> int bind(int sockfd, struct sockaddr *my_addr, int addrlen);
4- connect() hey you!!
5- listen() Ready to be connecto, someone call me
6- accept() ->
7- send() -> int send(int sockfd, const void *msg, int len, int flags); 
8- recv() -> int recv(int sockfd, void *buf, int len, int flags);
*/
void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


static void signal_handler ( int signal_number )
{
    /**
    * Save a copy of errno so we can restore it later.  See https://pubs.opengroup.org/onlinepubs/9699919799/
    * "Operations which obtain the value of errno and operations which assign a value to errno shall be
    *  async-signal-safe, provided that the signal-catching function saves the value of errno upon entry and
    *  restores it before it returns."
    */
    int errno_saved = errno;
    if ( signal_number == SIGINT ) {
        //caught_sigint = true;
        printf("Caught signal_handler SIGINT \n");
        close(sockett);
        fclose(fptr);
        remove(FILE_NAME);
        exit(0);
    } else if ( signal_number == SIGTERM ) {
        //caught_sigterm = true;
        printf("Caught signal_handler SIGTERM \n");
        close(sockett);
        fclose(fptr);
        remove(FILE_NAME);
        exit(0);
    }
    errno = errno_saved;
}

int main (void){

    struct addrinfo hints, *res, *p;
    int status;
    char ipstr[INET6_ADDRSTRLEN];
    char s[INET6_ADDRSTRLEN];
    //int sockett, new_fd, numbytes;
    int new_fd, numbytes;
    socklen_t addr_size;
    int yes=1;
    void *addr;
    char *ipver, *ptr;
    struct sockaddr_storage their_addr;
    struct sigaction sa;
    char buf[MAXBUFLEN];

    //Creamos la estructura para acomodar las conexiones
    memset(&hints,0,sizeof hints);
    hints.ai_family= AF_UNSPEC;
    hints.ai_socktype= SOCK_STREAM; //TCP-SOCK_STREAM
    hints.ai_flags = AI_PASSIVE;

    //1) Create getaddrinfo
    if((status=getaddrinfo(NULL,PORT,&hints,&res))==!0){
        fprintf(stderr,"getaddrinfo: %s\n", gai_strerror(status));
        return 2;
    }

    printf("IP addresess for %s:\n\n", WEB);

    for(p=res; p!=NULL; p = p->ai_next){

        if(p->ai_family == AF_INET){
            struct sockaddr_in *ipv4 = (struct  sockaddr_in*)p->ai_addr;
            addr = &(ipv4->sin_addr);
            ipver = "IPv4";
        }else{
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6*)p->ai_addr;
            addr = &(ipv6->sin6_addr);
            ipver = "IPv6";
        }

        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        printf(" %s: %s\n", ipver, ipstr);

        if((sockett = socket(res->ai_family,res->ai_socktype,res->ai_protocol))== -1){
            perror("LISTENER: socket");
            continue;
        }
        
        if(setsockopt(sockett,SOL_SOCKET, SO_REUSEADDR,&yes,sizeof yes)==-1){
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockett, res->ai_addr, res->ai_addrlen)== -1){
            close(sockett);
            perror("LISTENER:Error ocurred");
            continue;
        }
        break;
    }


    if (p==NULL){
        fprintf(stderr,"server: failed to bind \n");
        exit(1);
    }

    freeaddrinfo(res);

    if(listen(sockett,BACKLOG)==-1){
        perror("could no open the listener");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("Listening server: waiting for connections...\n");

    struct sigaction new_action;
    //bool success = true;
    memset(&new_action,0,sizeof(struct sigaction));
    new_action.sa_handler=signal_handler;
    if( sigaction(SIGTERM, &new_action, NULL) != 0 ) {
        printf("Error %d (%s) registering for SIGTERM",errno,strerror(errno));
        //success = false;
    }
    if( sigaction(SIGINT, &new_action, NULL) ) {
        printf("Error %d (%s) registering for SIGINT",errno,strerror(errno));
        //success = false;
    }


     // take this FD file and create outside in a function.
    int i;
    //FILE * fptr;
    char *filename = "/var/tmp/aesdsocketdata" ;
    //ptr="starting";

    //filename = aesdsocketdata;
    fptr = fopen(filename, "w+"); 
    //fptr=open(filename, O_RDWR | O_CREAT | O_APPEND, 0644 );
    if (fptr != NULL){
        syslog(LOG_INFO, "Correctly entered arguments");
        
    }else{
        syslog(LOG_ERR,"Missing Filename and Text");        
    }

    for(;;){
       
        //accepting an incoming connection:
        addr_size = sizeof their_addr;
        new_fd = accept(sockett, (struct sockaddr *)&their_addr, &addr_size);
        if (new_fd==-1){
            perror("accept3e");
            //continue;
        }

        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        printf("inet_ntop inside while %s: %s\n", ipver, ipstr);

        for(;;){
        
            if ((numbytes = recvfrom(new_fd, buf, MAXBUFLEN-1 , 0,
                (struct sockaddr *)&their_addr, &addr_size)) == -1) {
                perror("recvfrom");
                exit(1);
            }

            printf("Accepted connection from %s\n ",ipstr);

            printf("listener: got packet from %s\n",
            inet_ntop(their_addr.ss_family,get_in_addr((struct sockaddr *)&their_addr),s, sizeof s));

            printf("listener: packet is %d bytes long\n", numbytes);
            buf[numbytes] = '\0';
            //printf("listener: packet contains \"%s\"\n", buf);
            printf("listener: packet contains %s", buf);
            //ptr = (char*)malloc(MAXBUFLEN * sizeof(char));
            ptr = (char*)malloc(numbytes + 1);
            // Check if the memory has been successfully
            // allocated by malloc or not
            if (ptr == NULL) {
                printf("Memory not allocated.\n");
                exit(0);
            }
         
            if (!fork()) { // this is the child process
                close(sockett); // child doesn't need the listener
                ptr=buf;     //Inside the child fork I must do the c&p text and send it back to the client.
                fputs(ptr,fptr);
                if(ptr,'\n'){
                    if(fseek(fptr,0,SEEK_SET)==0){
                        char file_buf[1024];
                        int read_bytes;
                        while ((read_bytes = fread(file_buf, sizeof(char), sizeof file_buf,fptr)) > 0) {
                            //send(new_fd, file_buf, read_bytes, 0);
                            if (send(new_fd, file_buf,read_bytes, 0) == -1)
                                perror("send");
                        }
                        // if (send(new_fd, ptr,sizeof(ptr), 0) == -1)
                        // perror("send");
                    }
                }
                //printf("Longitud del array %d",strlen(ptr));
                close(new_fd);
                exit(0);
            }

        }
        fclose(fptr);
        close(sockett);
        free(ptr);
    }
return  0;


}