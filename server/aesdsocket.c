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

#define PORT "9000"  // the port users will be connecting to

#define BACKLOG 10   // how many pending connections queue will hold

#define WEB "www.sirclavt.shop"

#define MAXBUFLEN 100

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

int main (void){

struct addrinfo hints, *res, *p;
int status;
char ipstr[INET6_ADDRSTRLEN];
int sockett, new_fd, numbytes;
socklen_t addr_size;
int yes=1;
   void *addr;
    char *ipver;
struct sockaddr_storage their_addr;
    struct sigaction sa;
     char buf[MAXBUFLEN];




//Creamos la estructura para acomodar las conexiones
memset(&hints,0,sizeof hints);
hints.ai_family= AF_UNSPEC;
hints.ai_socktype= SOCK_STREAM;
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
        perror("server: socket");
        continue;
    }
    
    if(setsockopt(sockett,SOL_SOCKET, SO_REUSEADDR,&yes,sizeof yes)==-1){
        perror("setsockopt");
        exit(1);
    }

    if (bind(sockett, res->ai_addr, res->ai_addrlen)== -1){
        close(sockett);
        perror("Error ocurred");
        continue;
    }
    break;
}


freeaddrinfo(res);

if (p==NULL){
    fprintf(stderr,"server: failed to bind \n");
    exit(1);
}

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
bool success = true;
memset(&new_action,0,sizeof(struct sigaction));
new_action.sa_handler=signal_handler;
if( sigaction(SIGTERM, &new_action, NULL) != 0 ) {
    printf("Error %d (%s) registering for SIGTERM",errno,strerror(errno));
    success = false;
}
if( sigaction(SIGINT, &new_action, NULL) ) {
    printf("Error %d (%s) registering for SIGINT",errno,strerror(errno));
    success = false;
}

while(success){ //accept() connections

    //accepting an incoming connection:
    addr_size = sizeof their_addr;
    new_fd = accept(sockett, (struct sockaddr *)&their_addr, &addr_size);
    if (new_fd==-1){
        perror("accept");
        continue;
    }

    inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
    printf("%s: %s\n", ipver, ipstr);

     if ((numbytes = recvfrom(sockett, buf, MAXBUFLEN-1 , 0,
        (struct sockaddr *)&their_addr, &addr_size)) == -1) {
        perror("recvfrom");
        exit(1);
    }

    printf("Accepted connection from %s\n ",ipstr);

    

// take this FD file and create outside in a function.
        int i;
        FILE * fptr;
        char *filename = "/var/tmp/aesdsocketdata" ;


        //filename = aesdsocketdata;
        fptr = fopen(filename, "w+"); 
        if (fptr != NULL){
            syslog(LOG_INFO, "Correctly entered arguments");
            //syslog(LOG_DEBUG,"Writing %s to %s", argv[2], argv[1]);
          //  char *str = recv();
            // fputs(str,fptr);
            // fputs("\n",fptr);
            // fclose(fptr);
                //Pruebas
                fputs("This is c programming.", fptr);
                fputs("\n",fptr);  
                fputs("This is a system programming language\n.", fptr);
                fclose(fptr);
                //Pruebas - final


        }else{
            syslog(LOG_ERR,"Missing Filename and Text");        
        }

      if (!fork()) { // this is the child process
            close(sockett); // child doesn't need the listener
            if (send(new_fd, "Hello, world! \n", 13, 0) == -1)
                perror("send");
            close(new_fd);
            exit(0);
        }
        close(new_fd);  // parent doesn't need this



}


char *msg = "Hola Mundo Cruel";
int len, bytes_sent;

// while(1){

// len=strlen(msg);
// bytes_sent=send(sockett,msg,len,0);


// recv(int sockfd, void *buf, int len, int flags);
// }
// close(sockett);



}