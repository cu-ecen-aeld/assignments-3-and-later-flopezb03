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

#include <stdlib.h>

#define VARFILE_PATH "/var/tmp/aesdsocketdata"
#define BUFFER_SIZE 1024

int server_fd, client_fd;

void closeall(){
    if(server_fd != -1)
        close(server_fd);
    if(client_fd != -1)
        close(client_fd);
    remove(VARFILE_PATH);
    closelog();
}

void signal_handler(int sig){
    closeall();
    exit(EXIT_SUCCESS);
}

void set_sigaction(){
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}



int socket2file(int client_fd);
int file2socket(int client_fd);

int main(){
    struct addrinfo hints;
    struct addrinfo* server_addrinfo;
    struct sockaddr_in client_sockaddr;
    socklen_t client_addrlen;
    char ip4[INET_ADDRSTRLEN];

    // Set sigaction
    set_sigaction();

    // Open Syslog
    openlog(NULL, 0, LOG_USER);

    // Socket init
    server_fd = socket(PF_INET, SOCK_STREAM, 0);
    if(server_fd == -1){
        closeall();
        exit(EXIT_FAILURE);
    }

    /*
    int optval = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    */
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

    // Listen
    listen(server_fd, 1);

    while(1){
        
        client_addrlen = sizeof(client_sockaddr);
        client_fd = accept(server_fd, (struct sockaddr*)&client_sockaddr, &client_addrlen);
        if(client_fd == -1){
            closeall();
            exit(EXIT_FAILURE);
        }

        inet_ntop(AF_INET, &client_sockaddr.sin_addr, ip4, INET_ADDRSTRLEN);
        syslog(LOG_INFO, "Accepted connection from %s", ip4);

        if(socket2file(client_fd) == 0){
            closeall();
            exit(EXIT_FAILURE);
        }

        if(file2socket(client_fd) == 0){
            closeall();
            exit(EXIT_FAILURE);
        }

        syslog(LOG_INFO, "Closed connection from %s", ip4);
        close(client_fd);
    }
    closeall();
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
