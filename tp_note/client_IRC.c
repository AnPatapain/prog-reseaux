#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define PORT 1234
#define BUFFER_SIZE 1024

#define CLIENT_READY 0

void stop(char*);

void connect_to_server(int *sockfd, struct sockaddr_in* server_addr);

void chatting(int i, int sockfd);

int main() {
    // Create the client socket
    int sockfd, fdmax, i;
    // Address structure for server socket
    struct sockaddr_in server_addr;
    connect_to_server(&sockfd, &server_addr);

    // On crée le readfds contenant 0 et sockfd le 0 est réprésantant de cette client et sockfd est pour le serveur
	fd_set read_fds, actual_read_fds;
    FD_ZERO(&actual_read_fds);
    FD_ZERO(&read_fds);
    FD_SET(CLIENT_READY, &read_fds);
    FD_SET(sockfd, &read_fds);
	
    fdmax = sockfd;
    // chatting(sockfd);
    while(1) {
        actual_read_fds = read_fds;
        
        // On va utiliser select avec FD_ISSET pour détecter qui est prêt pour être lu ? le 0 ou le sockfd ( serveur )
        if(select(fdmax+1, &actual_read_fds, NULL, NULL, NULL) < 0) {
            stop("select error");
        }

        for(i=0; i <= fdmax; i++ ){
			if(FD_ISSET(i, &actual_read_fds)){
				chatting(i, sockfd);
			}
        }
    }

    close(sockfd);

    return 0;
}

void connect_to_server(int *sockfd, struct sockaddr_in* server_addr) {
    if( (*sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1 ) {
        stop("could not create a socket");
    }
    server_addr->sin_port = htons(PORT);
    server_addr->sin_family = AF_INET;
    server_addr->sin_addr.s_addr = inet_addr("127.0.0.1");
    if (connect(*sockfd, (const struct sockaddr*)server_addr, sizeof(struct sockaddr)) < 0) {
        stop("could not connect with server");
    }else {
        char nickname_buf[BUFFER_SIZE];
        printf("write your nickname: ");
        fgets(nickname_buf, BUFFER_SIZE, stdin);
        // Send the nickname of client to server
        if(send(*sockfd, nickname_buf, BUFFER_SIZE, 0) == -1) {
            stop("error when sending the nickname of client to the server");
        } 
    }
}

void chatting(int i, int sockfd)
{
	char send_buf[BUFFER_SIZE];
	char recv_buf[BUFFER_SIZE];
	int nbyte_recvd;
	
	if (i == 0){
		fgets(send_buf, BUFFER_SIZE, stdin);
		if (strcmp(send_buf , "quit\n") == 0) {
			exit(0);
		}else{
			write(sockfd, send_buf, strlen(send_buf));
        }
	}else {
		nbyte_recvd = recv(sockfd, recv_buf, BUFFER_SIZE, 0);
		recv_buf[nbyte_recvd] = '\0';
		printf("%s\n" , recv_buf);
		fflush(stdout);
	}
}

void stop(char *message) {
    perror(message);
    _Exit(1);
}
