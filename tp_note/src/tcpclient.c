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

void stop(char *message) {
    perror(message);
    _Exit(1);
}

int main() {
    char message_to_server[BUFFER_SIZE] = "PING";
    char message_from_server[BUFFER_SIZE];
    // Create the client socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    // Address structure for server socket
    struct sockaddr_in server_addr;
    
    if (sockfd < 0) {
        printf("could not create a socket");
    }

    // server address localhost and through port 1234 
    server_addr.sin_port = htons(PORT);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sockfd, (const struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        stop("could not connect with server");
    }

    int n;
    for (int i = 0; i < 1; i++) {
        if( send(sockfd, message_to_server, strlen(message_to_server), 0) < 0 ) {
            stop("cant send message from client to server");
        }
        n = recv(sockfd, message_from_server, strlen(message_from_server), 0);
        if(n < 0) {
            stop("cant receive message from server");
        }
        message_from_server[n] = '\0';

        printf("\nMessage from server: %s\n", message_from_server);

    }

    close(sockfd);

    return 0;
}
