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
    char message_to_client[BUFFER_SIZE] = "PONG";
    char message_from_client[BUFFER_SIZE];
    // Create Socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // Address structure for both server and client
    struct sockaddr_in server_addr, client_addr;

    int client_size = sizeof(client_addr);

    if (sockfd < 0) {
        stop("\nCould not create a tcp socket\n");
    }

    // port 1234s and localhost address
    server_addr.sin_port = htons(PORT);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Bind the address structure into the socket
    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr))  < 0){
        stop("could not bind the localhost address to server");
    }

    // Listen for maximum 5 incomming connection 
    if (listen(sockfd, 5) < 0) {
        stop("error occur when listenning incomming connection");
    }

    // Accept incomming connection if the queue not full and return the the sockfd of the accepted socket
    // The server will wait at this line untill the client call connect()
    int accepted_sockfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_size);
    
    if (accepted_sockfd < 0) {
        stop("error occur when accepting incomming connection"); 
    }
    printf("Client connected at IP: %s and port: %i\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    
    int n;
    for(int i = 0; i < 1000; i++) {
        // Wait and receive message of the accepted socket
        n = recv(accepted_sockfd, message_from_client, BUFFER_SIZE, 0);

        if(n < 0){
            stop("error occurs in recvfrom");
        }
        
        message_from_client[n] = '\0';
        // Print message of the accepted socket
        printf("\nmessage from clients: %s\n", message_from_client);
        // Send the message back to the accepted socket
        
        if( send(accepted_sockfd, message_to_client, strlen(message_to_client), 0) < 0 ) {
            stop("cant send message from server to client");
        }
    }
    close(accepted_sockfd);
    close(sockfd);

    return 0;
}
