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
#include <errno.h>

#define PORT 1234
#define BUFFER_SIZE 1024
#define TRUE 1
#define False 0
#define MAX_CLIENT 3

typedef struct clientNode {
    int client_sockfd;
    char *nickname;
    struct clientNode* next;
}clientNode;

void stop(char *message) {
    perror(message);
    _Exit(1);
}

void server_init(int *, struct sockaddr_in* );
void connection_accept(fd_set *readfds, int *fdmax, int master_sockfd, struct sockaddr_in* client_addr, clientNode** client_list);
void chatting(int i, fd_set *readfds, int master_sockfd, int fdmax);
void forward_message(int k, char* buffer, int nBytes);

int main() {
    int master_sockfd, fdmax;
    struct sockaddr_in server_addr, client_addr;
    clientNode *client_head = NULL;

    fd_set readfds, actual_readfds;
    FD_ZERO(&readfds);
    FD_ZERO(&actual_readfds);
    // Initialize server
    server_init(&master_sockfd, &server_addr);
    FD_SET(master_sockfd, &readfds);

    fdmax = master_sockfd;
    while(TRUE) {
        // We change the readfds au cours de body of while and at the beginning we assign readfds to actual_readfds
        actual_readfds = readfds;
        int max_sockfd = master_sockfd;
        
        if(select(fdmax + 1, &actual_readfds, NULL, NULL, NULL) == -1) {
            stop("error occurs when selecting the ready socket");
        }

        for (int i = 0; i <= fdmax; i++){
			if (FD_ISSET(i, &actual_readfds)){
				if (i == master_sockfd){
                    // If the master_sockfd is ready it means there is a client want to connect
                    // The readfds, fdmax can be change after this function call
                    connection_accept(&readfds, &fdmax, master_sockfd, &client_addr, &client_head);
                    clientNode* temp = client_head;
                    while(temp != NULL) {
                        printf("\n%d %s\n", temp->client_sockfd, temp->nickname);
                        temp = temp->next;
                    }
                }
				else{
                    printf("\nchatting mode\n");
                    chatting(i, &readfds, master_sockfd, fdmax);
                }
			}
		}       

    }

    return 0;
}

void server_init(int *master_sockfd, struct sockaddr_in *server_addr) {
    int opt = TRUE;

    // Create server address structure
    server_addr->sin_port = htons(PORT);
    server_addr->sin_family = AF_INET;
    server_addr->sin_addr.s_addr = inet_addr("127.0.0.1");

    // Create master socket
    if ( ( *master_sockfd = socket(AF_INET, SOCK_STREAM, 0) ) < 0) {
        stop("could not create a master socket");
    }

    // Allow multiple client to connect to the master server
    if ( setsockopt(*master_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int) ) == -1 ) {
        stop("Error occurs when setsockopt was called");
    }

    // Bind master socket to localhost and communicate with clients through PORT
    if (bind(*master_sockfd, (struct sockaddr*)server_addr, sizeof(struct sockaddr)) < 0) {
        stop("error occurs when binding the server_addr to master socket");
    }

    // Listen
    if (listen(*master_sockfd, 3) < 0) {
        stop("error occurs when listenning for incomming connection");
    }
    printf("\nServer listenning on port 1234\n");
	fflush(stdout);
}

void connection_accept(fd_set *readfds, int *fdmax, int master_sockfd, struct sockaddr_in* client_addr, clientNode** client_list) {
    socklen_t client_addr_len = sizeof(struct sockaddr_in);
    int client_sockfd = accept(master_sockfd, (struct sockaddr*)client_addr, &client_addr_len);
    
    if( client_sockfd == -1) {
        stop("error occurs when accepting the new client");
    }else {
        
        char nickname_buffer[BUFFER_SIZE];
        int nickname_size;
        
        // printf("\nclient index 0 avant read %s\n", client_list[0].nickname);
        if( ( nickname_size = recv(client_sockfd, nickname_buffer, BUFFER_SIZE, 0) ) == -1) {
            stop("could not receive nickname of the client");
        }
        nickname_buffer[nickname_size] = '\0';

        clientNode* node = (clientNode *)malloc(sizeof(clientNode));
        node->client_sockfd = client_sockfd;
        node->nickname = (char *)malloc(sizeof(char) * nickname_size);
        for(int i = 0; i <= nickname_size; i++) {
            node->nickname[i] = nickname_buffer[i];
        }
        if(*client_list == NULL) {
            *client_list = node;
            (*client_list)->next = NULL;
        }else {
            node->next = *client_list;
            *client_list = node;
        }
        
        // Put the new client into readfds we must modify the fdmax also
        FD_SET(client_sockfd, readfds);
        if(client_sockfd > *fdmax) {
            *fdmax = client_sockfd;
        }
        
        printf("client %d join to server\n", client_sockfd);
    }

    
}

void chatting(int i, fd_set *readfds, int master_sockfd, int fdmax) {
    char buffer[BUFFER_SIZE];
    int nBytes = read(i, buffer, BUFFER_SIZE);
    if (nBytes < 0) {
        stop("errors in chatting");
    }
    if(nBytes == 0) {
        printf("client %d disconnected\n", i);
        close(i);
        FD_CLR(i, readfds);
    }
    else if(nBytes > 0) {
        for(int k = 0; k <= fdmax; k++) {
            if(FD_ISSET(k, readfds) && k != i && k != master_sockfd) {
                // printf("message will be forwarded: %s from %d to %d\n", buffer, i, k);
                forward_message(k, buffer, nBytes);
            }
        }
    }
}

void forward_message(int k, char* buffer, int nBytes) {
    if(write(k, buffer, nBytes) == -1) {
        stop("could not forward message");
    }
}

