#define _POSIX_C_SOURCE 200112L

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <fcntl.h>
#include <syslog.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <signal.h>
#include <sys/queue.h>
#include "slist_foreach_safe.h"
#include <pthread.h>
#include <time.h>
#include <stdlib.h>



#define VARFILE_PATH "/var/tmp/aesdsocketdata"
#define BUFFER_SIZE 1024





int server_fd = -1;

SLIST_HEAD(slisthead, entry) head =
	   SLIST_HEAD_INITIALIZER(head);
struct entry {
    pthread_t t;
    int client_fd;
    char client_ip4[INET_ADDRSTRLEN];
    int end;

    SLIST_ENTRY(entry) entries;
};

pthread_mutex_t mutex;

int exit_signal = 0;




void closeall();

void close_signal_handler();
void timer_signal_handler();
void set_signals();
void set_timer();

void set_daemon_mode();
void init_server_socket();

void* thread_task(void* arg);
int socket2file(int client_fd);
int file2socket(int client_fd);






int main(int argc, char** argv){
    int daemon_mode = 0;

    struct sockaddr_in client_sockaddr;
    socklen_t client_addrlen;
    char ip4[INET_ADDRSTRLEN];

    struct entry *node_iterator, *node_iterator_tmp;



    // Init vars
    if (argc == 2 && strcmp(argv[1], "-d") == 0)
        daemon_mode = 1;

    pthread_mutex_init(&mutex,NULL);

    openlog(NULL, 0, LOG_USER);
    // Set sigaction
    set_signals();



    // Socket init
    init_server_socket();
    // Set daemon mode
    if(daemon_mode)
        set_daemon_mode();



    // Set timer
    set_timer();

    // Server starts listen
    listen(server_fd, 1024);

    while(1){

        // Delete finished threads
        SLIST_FOREACH_SAFE(node_iterator, &head, entries, node_iterator_tmp) {
            if (node_iterator->end) {
                pthread_join(node_iterator->t, NULL);

                close(node_iterator->client_fd);
                SLIST_REMOVE(&head, node_iterator, entry, entries);
                syslog(LOG_INFO, "Closed connection from %s", node_iterator->client_ip4);

                free(node_iterator);
            }
        }

        // Accept new client
        client_addrlen = sizeof(client_sockaddr);
        int new_client_fd = accept(server_fd, (struct sockaddr*)&client_sockaddr, &client_addrlen);
        if(new_client_fd == -1){
            if (exit_signal) 
                break;
            continue;
        }

        inet_ntop(AF_INET, &client_sockaddr.sin_addr, ip4, INET_ADDRSTRLEN);
        syslog(LOG_INFO, "Accepted connection from %s", ip4);


        // Create list node for the client
        struct entry* new_node = malloc(sizeof(struct entry));
        new_node->client_fd = new_client_fd;
        strcpy(new_node->client_ip4, ip4);
        new_node->end = 0;

        // Init thread and insert node in the list
        if(pthread_create(&new_node->t, NULL, thread_task, new_node) == 0)
            SLIST_INSERT_HEAD(&head, new_node, entries);

    }

    SLIST_FOREACH_SAFE(node_iterator, &head, entries, node_iterator_tmp){
        pthread_join(node_iterator->t, NULL);

        close(node_iterator->client_fd);
        SLIST_REMOVE(&head, node_iterator, entry, entries);
        syslog(LOG_INFO, "Closed connection from %s", node_iterator->client_ip4);

        free(node_iterator);
    }
    closeall();
    
    exit(EXIT_SUCCESS);
}



void closeall(){
    if(server_fd != -1)
        close(server_fd);
    remove(VARFILE_PATH);
    pthread_mutex_destroy(&mutex);
    closelog();
}

void close_signal_handler(){
    exit_signal = 1;
}

void timer_signal_handler(){
    time_t now;
    struct tm *timeinfo;
    char buffer[128];
    int fd;

    // Get timestamp
    time(&now);
    timeinfo = localtime(&now);

    // Format time with RFC 2822
    strftime(buffer, sizeof(buffer), "timestamp:%a, %d %b %Y %T %z\n", timeinfo);

    // Append timestamp
    pthread_mutex_lock(&mutex);
    fd = open(VARFILE_PATH, O_WRONLY | O_APPEND | O_CREAT, 0640);
    write(fd, buffer, strlen(buffer));
    pthread_mutex_unlock(&mutex);

    close(fd);
}

void set_signals(){
    struct sigaction sa;
    sa.sa_handler = close_signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    sa.sa_handler = timer_signal_handler;
    sigaction(SIGALRM, &sa, NULL);
}

void set_timer(){
    timer_t timer10s;
    struct itimerspec timer_interval;

    timer_interval.it_value.tv_sec = 10;
    timer_interval.it_value.tv_nsec = 0L;
    timer_interval.it_interval.tv_sec = 10;
    timer_interval.it_interval.tv_nsec = 0L;

    timer_create(CLOCK_REALTIME, NULL, &timer10s);
    timer_settime(timer10s, 0, &timer_interval, NULL);
}

void set_daemon_mode(){
    pid_t pid = fork();
        if(pid < 0){
            closeall();
            exit(EXIT_FAILURE);
        }

        if(pid > 0){
            closeall();
            exit(EXIT_SUCCESS);
        }

        setsid();
        chdir("/");
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
}

void init_server_socket(){
    struct addrinfo hints;
    struct addrinfo* server_addrinfo;
    int optval = 1;


    server_fd = socket(PF_INET, SOCK_STREAM, 0);
    if(server_fd == -1){
        closeall();
        exit(EXIT_FAILURE);
    }



    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if(getaddrinfo(NULL, "9000", &hints, &server_addrinfo) != 0){
        closeall();
        exit(EXIT_FAILURE);
    }

    if(bind(server_fd, server_addrinfo->ai_addr, server_addrinfo->ai_addrlen) != 0){
        closeall();
        freeaddrinfo(server_addrinfo);
        exit(EXIT_FAILURE);
    }
    freeaddrinfo(server_addrinfo);
}

void* thread_task(void* arg){
    struct entry* node = arg;

    pthread_mutex_lock(&mutex);
    socket2file(node->client_fd);
    file2socket(node->client_fd);
    pthread_mutex_unlock(&mutex);
    node->end = 1;
    pthread_exit(NULL);
}

int socket2file(int client_fd){
    char buffer[BUFFER_SIZE];
    int bytes_read;
    int var_fd;

    var_fd = open(VARFILE_PATH, O_WRONLY | O_CREAT | O_APPEND, 0640);
    if(var_fd == -1)
        return 0;

    do{
        bytes_read = recv(client_fd, buffer, BUFFER_SIZE, 0);

        write(var_fd, buffer, bytes_read);
    }while(buffer[bytes_read-1] != '\n');

    close(var_fd);
    return 1;
}

int file2socket(int client_fd){
    char buffer[BUFFER_SIZE];
    int size_read;
    int var_fd;

    var_fd = open(VARFILE_PATH, O_RDONLY, 0440);
    if(var_fd == -1)
        return 0;

    while((size_read = read(var_fd, buffer, BUFFER_SIZE)) != 0)
        send(client_fd, buffer, size_read, 0);

    close(var_fd);
    return 1;
}

