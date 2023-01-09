#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define PORT 1234

void stop(char *message) {
    perror(message);
    _Exit(0);
}

int main() {
    int sockfd;
    char buffer[1024];
    char *message_to_server = "PING";
    struct sockaddr_in servaddr;

    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        stop("could not create a socket");
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int n, len;

    while(1) {
        sendto(sockfd, (const char *)message_to_server, strlen(message_to_server), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
        n = recvfrom(sockfd, (char *)buffer, 1024, MSG_WAITALL, (struct sockaddr *) &servaddr, &len);
        buffer[n] = '\0';
        printf("Server reply: %s\n", buffer);
    }
    return 0;
}