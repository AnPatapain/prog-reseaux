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

#define BUFFER_SIZE 8096 

void stop(char *message) {
    perror(message);
    _Exit(0);
}

int main() {
    int portno = 1234;
    struct sockaddr_in serv_addr, client_addr;

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) { stop("could not create socket"); }

    serv_addr.sin_port = htons(portno);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (bind(sockfd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr))  == -1){
        stop("could not bind the localhost address to server");
    }

    char buffer[BUFFER_SIZE];
    char message_to_client[5] = "PONG";
    int len = sizeof(client_addr);
    while(1) {
        
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &len);
        if(n == -1){
            stop("error occurs in recvfrom");
        }
        buffer[n] = '\0';

        printf("\nClient message: %s\n", buffer);

        printf("client Info: IP: %d, Port: %hu", client_addr.sin_addr.s_addr, client_addr.sin_port);

        sleep(1);

        sendto(sockfd, (const char *)message_to_client, strlen(message_to_client), MSG_CONFIRM, (const struct sockaddr *) &client_addr, sizeof(client_addr));
    }

    return 0;
}