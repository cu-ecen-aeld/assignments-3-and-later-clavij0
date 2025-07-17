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
#include <sys/stat.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include "../aesd-char-driver/aesd_ioctl.h"

#define PORT "9000"  // the port users will be connecting to

#define BACKLOG 10   // how many pending connections queue will hold

#define MAXBUFLEN 1024 //set buffer lenght

#define MAXBUFLEN_SIZE 2048 //Max packet size 

//#define FILE_NAME "/var/tmp/aesdsocketdata"

#define USE_AESD_CHAR_DEVICE 1

#if (USE_AESD_CHAR_DEVICE)
#define FILE_NAME  "/dev/aesdchar"
#else
#define FILE_NAME  "/var/tmp/aesdsocketdata"
#endif

pthread_mutex_t log_mutex;

bool bool_pthread = 1;

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

struct thread_data
{
    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    int new_fd;    
};

struct node_thread
{
    pthread_t thread_id;
    struct node_thread *next;
};

struct node_thread* head;

void Insert(pthread_t thread_id_node)
{   
    struct node_thread* node_data = (struct node_thread*)malloc(sizeof(struct node_thread));
    node_data->thread_id = thread_id_node;
    printf("Insert Thread ID %lu \n" , node_data->thread_id);
    node_data->next = head;
    head=node_data;
   // free(node_data);
}

void Print()
{
    struct node_thread* temp = head;
    printf("List id of the nodes are: ");
    //printf("asdas %lu" , temp->thread_id);
    while(temp != NULL)
    {
        printf(" %lu \n" , temp->thread_id);
        temp=temp->next;
    }
}

void freeList()
{
   struct node_thread* tmp = head;

   if (head == NULL){
    printf("NO head\n");
    free(tmp);
    return;
   }

   while (head != NULL)
    {
        tmp = head;
        head = head->next;
        //printf("Node data %lu\n", tmp->thread_id);
        syslog(LOG_INFO, "Pthread Node Created data %lu", tmp->thread_id); //Just for my control
        pthread_join(tmp->thread_id, NULL);
        free(tmp);
    }

}

void Delete_thread()
{
   struct node_thread* tmp1 = head;

   while (head != NULL)
    {
       head = head->next;
       free(tmp1);
    }

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
    if ( signal_number == SIGINT ||  signal_number == SIGTERM  ) {
        //printf("Caught signal, exiting SIGINT || SIGTERM\n");
        syslog(LOG_INFO,"Caught signal, exiting SIGINT || SIGTERM\n"); //Just for my control
        bool_pthread = false;

    }
    errno = errno_saved;
}


void daemonize() {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        // Parent process exits
        exit(EXIT_SUCCESS);
    }

    // Child process continues

    // Create a new session
    if (setsid() < 0) {
        perror("setsid failed");
        exit(EXIT_FAILURE);
    }

    // Set the file mode creation mask to 0
    umask(0);

    // Redirect standard file descriptors to /dev/null
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    open("/dev/null", O_RDWR);
    dup(0);
    dup(0);
}
 
// Function to append timestamp to the log file
void *append_timestamp(void *arg) {
    while (bool_pthread) {
        // Sleep for 10 seconds
        sleep(10);

        time_t now = time(NULL);
        struct tm *timeinfo = localtime(&now);
        char timestamp[64];

        // Format the time in RFC 2822 format
        strftime(timestamp, sizeof(timestamp), "timestamp:%a, %d %b %Y %H:%M:%S %z\n", timeinfo);

        // Lock before writing the timestamp
        pthread_mutex_lock(&log_mutex);

        // Open the file to append the timestamp
        // Is in the main the open FILE_NAME
        //fputs(timestamp, fptr);
        // Unlock after writing
        pthread_mutex_unlock(&log_mutex);
        // printf("\nFree Time Mutex\n");
    }
    return NULL;
}


void *parnert_handler(void *thread_param){

    struct thread_data* thread_func_args = (struct thread_data *)thread_param;
    char s[INET6_ADDRSTRLEN];
    inet_ntop(thread_func_args->their_addr.ss_family,get_in_addr((struct sockaddr *)&thread_func_args->their_addr),s, sizeof s);
    
    //inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
    syslog(LOG_INFO, "Accepted connection from %s", s);
    printf("Accepted connection from %s \n", s);
    // Receiving data
    char buf[1024] = {0};
    int numbytes=0;   
    char *ptr = NULL;
    //FILE * fptr;

    unsigned int write_wd;
    unsigned int write_offset;
    struct aesd_seekto seekto;

    while(1){
        printf("vuelta al mundo \n");
        numbytes = recvfrom(thread_func_args->new_fd, buf, MAXBUFLEN-1 , 0,(struct sockaddr *)&thread_func_args->their_addr, &thread_func_args->addr_size);        

        if (numbytes == -1){
            syslog(LOG_ERR, "ERROR recv() failed-Closed connection from %s", s);
            //perror("recv() failed");
            break;
        }else if(numbytes == 0){
            //printf("wWHILE 2 - Closed connection from %s \n",s);
            syslog(LOG_INFO, "Closed connection from %s", s);
            close(thread_func_args->new_fd);
            break;
        }
        syslog(LOG_INFO, "Inside parnert_handler funtion-> recfrom"); //Just for my control

        //printf("Numbytes %d 1\n", numbytes);
        if (numbytes>MAXBUFLEN_SIZE){
            syslog(LOG_ERR, "Packet too large (%d bytes), discarding", numbytes);
            // Discard the packet and continue
            printf("error numbytes numbytes>MAXBUFLEN_SIZE\n");

            return 0;
        }
        if (pthread_mutex_lock(&log_mutex) != 0)
        {
            perror("Unable to lock pthread_mutex");
        }else{
            printf("FORWARD inside MUTEX\n");
        }

        printf("Numbytes %d 2\n", numbytes);

        ptr = (char*)malloc(sizeof(char)*(MAXBUFLEN + 1));
        // // Check if the memory has been successfully
        // // allocated by malloc or not
        if (ptr == NULL) {
            printf("Memory not allocated.\n");
            syslog(LOG_ERR,"Failed Memory not allocated.");
            pthread_mutex_unlock(&log_mutex);
            exit(0);
        }
        
        memcpy(ptr,buf, numbytes);//dont forget to c&p the buf recv to the alloc memory
        ptr[numbytes] = '\0';
        //syslog(LOG_INFO,"Received %d bytes: %.*s",numbyte,numbytes,buf);

        //--------------ASSIGNMENT 9 BEGINING-----------//
        // Hasta el byte 19 quiero asegurarme que tiene los :
        int cmp = strncmp(ptr,"AESDCHAR_IOCSEEKTO:", 19);
        syslog(LOG_INFO, "compera cmp  %d",cmp);
        if(strncmp(ptr,"AESDCHAR_IOCSEEKTO:", 19)==0){
            printf("Command AESDCHAR_IOCSEEKTO detected\n");
            
            syslog(LOG_INFO, "Detected AESDCHAR_IOCSEEKTO command %s",ptr);

            //fptr = fopen(FILE_NAME, "r+"); 
            //int fd = fileno(fptr);
            /*Remmenber NEVER use fopen() and its friends when USE ioctl(), poll(), select()
            */
            int fd = open(FILE_NAME, O_RDWR);

            if (fd < 0) {
                syslog(LOG_ERR, "open() failed: %s", strerror(errno));
                free(ptr);
                pthread_mutex_unlock(&log_mutex);
                break;
            }

            if(sscanf(ptr,"AESDCHAR_IOCSEEKTO:%u,%u",&write_wd,&write_offset)==2){
                seekto.write_cmd = write_wd;
                seekto.write_cmd_offset = write_offset;
                printf("write_wd %u write_cmd_offset %u \n",seekto.write_cmd , seekto.write_cmd_offset);
                syslog(LOG_INFO, "Detected AESDCHAR_IOCSEEKTO command write_wd %u write_cmd_offset %u \n",seekto.write_cmd , seekto.write_cmd_offset);
            }else{
                syslog(LOG_ERR,"ioctl AESDCHAR_IOCSEEKTO %s",strerror(errno));
                printf("No se puedo ejecutar sscanf\n");
                //fclose(fptr);
                close(fd);
                free(ptr);
                pthread_mutex_unlock(&log_mutex);
                break;
            }

            if(ioctl(fd,AESDCHAR_IOCSEEKTO,&seekto)<0){
                syslog(LOG_ERR,"ioctl ERROR AESDCHAR_IOCSEEKTO %s",strerror(errno));
                printf("No se puedo ejecutar ioctl\n");
                //fclose(fptr);
                close(fd);
                free(ptr);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      
                pthread_mutex_unlock(&log_mutex);
                break;
            }else{
                 // 🔧 Re-sincronizar el FILE* con el fd actualizado
                //fseek(fptr, 0, SEEK_CUR);
                //Read since the given position
                printf("ioctl enviados y listo para el nuevo filp->f_pos\n");
                char file_buf[MAXBUFLEN]={0};
                ssize_t read_bytes ;
                //Control de posición
                //long pos = ftell(fptr);

                //printf("1 Nueva posición del archivo tras ioctl: %ld\n", ftell(fptr));

                //char preview_buf[1024] = {0};
                // Leer una pequeña porción para depurar/verificar
                //size_t preview_bytes = fread(preview_buf, sizeof(char), sizeof(preview_buf) - 1, fptr);

                //printf("Preview antes del bucle: [%s] (%zu bytes)\n", preview_buf, preview_bytes);

                // IMPORTANTE: regresar el cursor al punto original antes del bucle
                //fseek(fptr, -((long)preview_bytes), SEEK_CUR);
                
                //printf("read_bytes # %lu\n",read_bytes);             
                //read_bytes = fread(file_buf,sizeof(char),numbytes,fptr);
                //printf("read_bytes # %lu\n",read_bytes);
                //read_bytes =0;
                //printf("read_bytes # %lu\n",read_bytes);
                //OJO aquí
                //if (read_bytes > 0){
                //if ((read_bytes = fread(file_buf, sizeof(char), sizeof(numbytes),fptr)) > 0){
                //while ((read_bytes = read(file_buf, sizeof(char), sizeof(numbytes),fd)) > 0) {
                while ((read_bytes = read(fd,file_buf,sizeof(file_buf))) > 0) {
                    printf("WHILE read_bytes # %lu\n",read_bytes);
                    printf("dentro de read_bytes Lectura nueva posición\n");
                    if(send(thread_func_args->new_fd,file_buf,read_bytes,0 )< 0){
                        syslog(LOG_ERR,"Send failed %s",strerror(errno));
                        printf("Faild sending buffer AESDCHAR_IOCSEEKTO\n");
                    }
                    if(pthread_mutex_unlock(&log_mutex) != 0){
                    perror("Unable to unlock pthread_mutex");
                    }else{
                        printf("Ioctl OUT MUTEXT \n");
                        syslog(LOG_INFO,"Ioctl OUT MUTEXT 2\n"); //Just for my control
                    }
                }/*else{
                    printf("Error read_bytes failed after ioctl \n");
                    syslog(LOG_ERR, "Read failed after ioctl: %s", strerror(errno));
                }*/

                free(ptr);   
                printf("Salida WHILE lectura ioctl()\n");
                //fclose(fptr);
                close(fd);
            }
        //free(ptr);   
        //printf("Salida ioctl()\n");
        //fclose(fptr);
        //pthread_mutex_unlock(&log_mutex);
        //--------------ASSIGNMENT 9 FINISH-----------//
        }else{
            syslog(LOG_INFO,"OPTION 2 no AESDCHAR_IOCSEEKTO: =) 2\n");
            int cmp = strncmp(ptr,"AESDCHAR_IOCSEEKTO:", 19);
            syslog(LOG_INFO, "compera cmp  %d",cmp);
            // fptr = fopen(FILE_NAME, "r+"); 
   
            // if (fptr != NULL){
            //     syslog(LOG_INFO, "Correctly entered arguments");
                
            // }else{
            //     syslog(LOG_ERR,"Missing Filename and Text %s\n", strerror(errno));        
            // }
            // if( fwrite (ptr,sizeof(char),numbytes,fptr) < numbytes){
            //     syslog(LOG_ERR,"Could not write into FD %s\n", strerror(errno));
            // }
    
            
            // if (fclose(fptr) == EOF){
            //     syslog(LOG_ERR,"Error Closing FD\n");
            // }else{
            //     syslog(LOG_INFO,"Success clossing\n");
            // }

            //char file_buf[MAXBUFLEN]={0};
            ssize_t write_bytes ;
            int fd = open(FILE_NAME, O_RDWR | O_CREAT | O_APPEND, 0644);

            if(fd < 0){
                syslog(LOG_ERR,"File opened failed %s",strerror(errno));
                return NULL;
            }else{
                  syslog(LOG_INFO, "Correctly File opened for writing");
            }

            //printf("Written to file: %s\n", ptr);
            if( (write_bytes = write(fd,ptr,numbytes)) < 0){
                syslog(LOG_ERR,"Could not write into FD %s\n", strerror(errno));
            }else if (sizeof(write_bytes)< numbytes){
                syslog(LOG_ERR,"Parcial write commands %zd of %d bytes \n",write_bytes,numbytes);
            }

            if (close(fd)== -1){
                syslog(LOG_ERR,"Error Closing FD %s\n",strerror(errno));
            }else{
                syslog(LOG_INFO,"Success clossing TOR\n");
            }
        
            if(strchr(ptr,'\n')){
                syslog(LOG_INFO,"Inside ptr\n");

                char file_buf[MAXBUFLEN]={0};
                int read_bytes=0;

                // fptr = fopen(FILE_NAME, "r+"); 

                // if (!fptr) {
                // syslog(LOG_ERR,"File is not opened %s",strerror(errno));
                // fptr = fopen(FILE_NAME, "a+");
                //     if (!fptr) {
                //         perror("Failed to open file");
                //         exit(EXIT_FAILURE);
                //     }
                // }
                int fd = open(FILE_NAME,O_RDWR | O_APPEND);
                if (fd < 0) {
                    syslog(LOG_ERR, "open() failed: %s", strerror(errno));
                    free(ptr);
                    pthread_mutex_unlock(&log_mutex);
                    break;
                }
                //Aquí empezar a escribir los datos nuevos.
                
                //while ((read_bytes = fread(file_buf, sizeof(char), numbytes,fptr)) > 0) {
                while ((read_bytes = read(fd,file_buf,sizeof(file_buf))) > 0) {
                    printf("read_bytes # %u\n",read_bytes);
                    syslog(LOG_INFO,"Read bytes %u",read_bytes);

                    if (send(thread_func_args->new_fd, file_buf,read_bytes, 0) < 0){
                        syslog(LOG_ERR,"Send action failed %s",strerror(errno));
                        break;
                    } 
                    if(pthread_mutex_unlock(&log_mutex) != 0){
                        perror("Unable to unlock pthread_mutex");
                    }else{
                        printf("OUT MUTEXT 2\n");
                        syslog(LOG_INFO,"OUT MUTEXT 2\n"); //Just for my control
                    }
                }

                free(ptr);   
                printf("Salida 1\n");
                //fclose(fptr);
                close(fd);
            }else{
                printf("Detected long_file.txt\n");
                pthread_mutex_unlock(&log_mutex);
            }
        }//end of else

    }//end of while
      
    if (numbytes == 0) {
        printf("Client disconnected\n");
    } else if (numbytes == -1) {
        perror("Receive failed");
    }

    close(thread_func_args->new_fd);
    free(thread_func_args);

    printf("Salida 2\n");
    return NULL;
}

int main (int argc, char *argv[]){
    
    int daemon_mode = 0;

    // Parse command line arguments
    if (argc == 2 && strcmp(argv[1], "-d") == 0) {
        daemon_mode = 1;
    }

    struct addrinfo hints, *res, *p;
    int status;
    int sockett;
    int new_fd;

    char ipstr[INET6_ADDRSTRLEN];

    socklen_t addr_size;
    int yes=1;
    void *addr;
    char *ipver;
    struct sockaddr_storage their_addr;
    struct sigaction sa;

     if (pthread_mutex_init(&log_mutex, NULL) != 0) {
        printf("Mutex init failed\n");
        return 1;
    }

    openlog("aesdsocket", LOG_PID | LOG_CONS, LOG_DAEMON);

    //Creamos la estructura para acomodar las conexiones
    memset(&hints,0,sizeof hints);
    hints.ai_family= AF_UNSPEC;
    hints.ai_socktype= SOCK_STREAM; //TCP-SOCK_STREAM
    hints.ai_flags = AI_PASSIVE;

    //1) Create getaddrinfo
    if((status=getaddrinfo(NULL,PORT,&hints,&res))!=0){
        fprintf(stderr,"getaddrinfo: %s\n", gai_strerror(status));
        return 1;
    }

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
            //return -1;
            continue;
        }
        
        if(setsockopt(sockett,SOL_SOCKET, SO_REUSEADDR,&yes,sizeof yes)==-1){
            perror("setsockopt(SO_REUSEADDR) failed");
            exit(1);
        }

        if (bind(sockett, res->ai_addr, res->ai_addrlen)== -1){
            close(sockett);
            perror("LISTENER:Error ocurred bind");
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
    //bool success = true;
    memset(&new_action,0,sizeof(struct sigaction));
    new_action.sa_handler=signal_handler;
    if( sigaction(SIGTERM, &new_action, NULL) != 0 ) {
        printf("Error %d (%s) registering for SIGTERM",errno,strerror(errno));
        //success = false;
    }
    if( sigaction(SIGINT, &new_action, NULL) ) {
        printf("Error %d (%s) registering for SIGINT",errno,strerror(errno));
    }

    // Run as a daemonpthread_d if -d argument is present
    if (daemon_mode) {
        daemonize();
    }

    // Create a thread to periodically append the timestamp
    //Timestamp REMOVED in assignment-8
      pthread_t timestamp_thread;
    if(!USE_AESD_CHAR_DEVICE){
        printf("Timestamp activeted\n");
        if (pthread_create(&timestamp_thread, NULL, append_timestamp, NULL) != 0) {
            perror("Could not create timestamp thread");
            return 1;
        }    
    }
    head = NULL;
    

    while(bool_pthread){

        printf("Bool_pthread stated %d \n", bool_pthread);
        //accepting an incoming connection:
        addr_size = sizeof their_addr;
        new_fd = accept(sockett, (struct sockaddr *)&their_addr,(socklen_t *)&addr_size);
        if (new_fd==-1){
            if (bool_pthread) {
                perror("No accepted new_fd Accept failed");
            }
            continue;
        }

        pthread_t client_thread;

        struct thread_data* data = (struct thread_data*)malloc(sizeof(struct thread_data));
        data->their_addr=their_addr;
        data->addr_size=addr_size;
        data->new_fd=new_fd;
        //data->mutex=mutex;

            if (pthread_create(&client_thread,NULL,parnert_handler,(void*)data) != 0 )
            {
                perror("Failed to create thread \n");
                free(data);
                return false;
            }else{
                printf("Pthread Created %lu \n" , client_thread);
                //Dont free here malloc -> data
            }
            
            Insert(client_thread);

        //printf("Inside While print\n");
        syslog(LOG_INFO,"Inside While of creating the client-thread");//Just for my control
        
    }
    //close(new_fd);
    //Timestamp REMOVED in assignment-8
    
    if(!USE_AESD_CHAR_DEVICE){
        pthread_join(timestamp_thread, NULL);
        remove(FILE_NAME);
    }
    syslog(LOG_INFO,"OUT OF WHILE remove FILE_NAME");//Just for my control
    freeList();
    close(sockett);
    closelog();
  
   return 0;
}