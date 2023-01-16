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

//Function for server-client connection and communication in chatroom handling
void server_init(int *, struct sockaddr_in* );
void connection_accept(fd_set *readfds, int *fdmax, int master_sockfd, struct sockaddr_in* client_addr, clientNode** client_head);
void chatting(int i, fd_set *readfds, int master_sockfd, int fdmax, clientNode* client_head);
void forward_message(int k, char* buffer, int nBytes);
void request_nickname(clientNode *client_head, int client_sockfd, char* nickname_buffer, int* nickname_len);
void message_formatted(char* buffer, char* prefix, char* buffer_formatted);
char **split_command(char *commande);
void command_handler(char** args, int client_sockfd, char* nickName, clientNode* client_head, fd_set *readfds);
void send_private_message(clientNode* client_head, char* nickName_src, int sockfd_src, char** args);

// Function for buffer handling
int remove_enter_in_buffer(char* buffer);
void clean_worlds_array(char **args);

// Function for client_list handling
void add_clientNode_to_list(clientNode**client_head, int client_sockfd, char* nickname_buffer, int nickname_len);
int check_nickname_valid(clientNode* client_head, char*nickname_buffer);
clientNode* find_client_by_sockfd(clientNode* client_head, int sockfd);
clientNode* find_client_by_nickname(clientNode* client_head, char* nickname);
void change_nickname(clientNode* client, int client_sockfd, char* new_nickname);

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
                    // while(temp != NULL) {
                    //     printf("\n%d %s\n", temp->client_sockfd, temp->nickname);
                    //     temp = temp->next;
                    // }
                }
				else{
                    chatting(i, &readfds, master_sockfd, fdmax, client_head);
                }
			}
		}       

    }

    return 0;
}

void server_init(int *master_sockfd, struct sockaddr_in *server_addr) {
    /*
    Initialize the server by binding the server_addr to master_sockfd and start listenning for the new connection
    */
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

void connection_accept(fd_set *readfds, int *fdmax, int master_sockfd, struct sockaddr_in* client_addr, clientNode** client_head) {
    /*
    Accept connection. The server must request client for the nickname and then add the new client to client_list
    and add client_sockfd to readfds
    */
    socklen_t client_addr_len = sizeof(struct sockaddr_in);
    int client_sockfd = accept(master_sockfd, (struct sockaddr*)client_addr, &client_addr_len);
    
    if( client_sockfd == -1) {
        stop("error occurs when accepting the new client");
    }else {
        
        char nickname_buffer[BUFFER_SIZE];
        int n, nickname_len;
        bzero(nickname_buffer, BUFFER_SIZE);

        // Request nickname of client
        request_nickname(*client_head, client_sockfd, nickname_buffer, &nickname_len);

        // Add clientNode to client_list
        add_clientNode_to_list(client_head, client_sockfd, nickname_buffer, nickname_len);

        // Put the new client into readfds we must modify the fdmax also
        FD_SET(client_sockfd, readfds);
        if(client_sockfd > *fdmax) {
            *fdmax = client_sockfd;
        }
        
        printf("client %d aka %s join to server\n", client_sockfd, nickname_buffer);
    }

    
}

void chatting(int i, fd_set *readfds, int master_sockfd, int fdmax, clientNode* client_head) {
    /*
    Chatting between server and client. There are two big case.
    Case1: the message from client start with / => server must handle the command
    Case2: the message from client is normal message => server must forward it to all clients in readfds except for
    the one that just has sent the message
    */
    
    // Find which client in client list that have sent the message
    char *nickName = (char *)malloc(sizeof(char) * BUFFER_SIZE);
    bzero(nickName, BUFFER_SIZE);
    clientNode* client = find_client_by_sockfd(client_head, i);
    strcpy(nickName, client->nickname);

    char buffer[BUFFER_SIZE];

    // Read the message from the client
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
        if(buffer[0] == '/') {
            // printf("\nbuffer in chatting: %s\n", buffer); ->>> BUG HEREEEEEEEEEEEEEE
            remove_enter_in_buffer(buffer);
            char **args = split_command(buffer);
            clean_worlds_array(args);
            //Test whether the args is clean or not
            // int o_ = 2;
            // while(args[o_] != NULL) {
            //     printf("\nlen: %d args[%d]: %s\n", strlen(args[o_]), o_, args[o_]);
            //     o_++;
            // }
            // The args is not clean before moving into command_handler
            command_handler(args, i, nickName, client_head, readfds);

        }else {
            char* buffer__ = (char*)malloc(sizeof(char)*BUFFER_SIZE);
            bzero(buffer__, BUFFER_SIZE);
            message_formatted(buffer, nickName, buffer__);
            printf("%s\n",buffer__);

            for(int k = 0; k <= fdmax; k++) {
                if(FD_ISSET(k, readfds) && k != i && k != master_sockfd) {
                    // printf("message will be forwarded: %s from %d to %d\n", buffer, i, k);
                    forward_message(k, buffer__, strlen(buffer__));
                }
            }
        }

    }
}

char **split_command(char *line)
{
    /*
    Split line into multiple worlds with delimiter is space. Then return a array where
    the world is stored
    */
    char **tokens = (char**)calloc(sizeof(char*), BUFFER_SIZE);
    for(int i = 0; i < BUFFER_SIZE; i++) {
        tokens[i] = (char*)calloc(sizeof(char), BUFFER_SIZE);
    }
    int i = 0; // for traversing line
    int j = 0; // for traversing tokens
    int k = 0; // for traversing tokens[j]

    while(line[i] != '\0') {
        if(line[i] == ' ') {
            tokens[j][k] = '\0';
            j++;
            k = 0;
        }else {
            tokens[j][k++] = line[i];
        }
        i++;
    }
    tokens[j][k] = '\0';

    for(int i = j+1; i < BUFFER_SIZE; i++) {
        tokens[i] = NULL;
    }
    
    return tokens;
}

void command_handler(char** args, int client_sockfd, char* nickName, clientNode* client_head, fd_set *readfds) {
    /*
    Handle diverse command by calling the corresponding function based upon the command args
    */
    if(strcmp(args[0], "/nickname") == 0 && args[1] != NULL) {
        char* new_nickname = args[1];
        change_nickname(client_head, client_sockfd, new_nickname);
    }
    if(strcmp(args[0], "/mp") == 0 && args[1] != NULL && args[2] != NULL) {
        send_private_message(client_head, nickName, client_sockfd, args);
    }
    
}

void send_private_message(clientNode* client_head, char* nickName_src, int sockfd_src, char** args){
    clientNode* client_node = find_client_by_nickname(client_head, args[1]);

    if(client_node == NULL) {
        char* error_message = (char*)malloc(sizeof(char)*BUFFER_SIZE);
        bzero(error_message, BUFFER_SIZE);
        strcpy(error_message, "sorry we could not find the client you want to send the message to!");
        
        if(write(sockfd_src, error_message, BUFFER_SIZE) == -1) {
            stop("could not send the private messaging error message to client");
        }
    }
    else {
        char *private_msg = (char*)malloc(sizeof(char) * BUFFER_SIZE);
        bzero(private_msg, BUFFER_SIZE);
        sprintf(private_msg, "%s%s", args[2], " ");
        int i_ = 3;
        while(args[i_] != NULL){
            if(i_ == 3) {
                sprintf(private_msg, "%s%s", private_msg, args[i_]);
            }else {
                sprintf(private_msg, "%s%s%s", private_msg, " ", args[i_]);
            }
            i_++;
        }
        char* prefix = (char*)malloc(sizeof(char) * BUFFER_SIZE);
        bzero(prefix, BUFFER_SIZE);
        sprintf(prefix, "%s%s", "private msg from: ", nickName_src);

        char* private_buffer = (char*)malloc(sizeof(char) * BUFFER_SIZE);
        bzero(private_buffer, BUFFER_SIZE);

        message_formatted(private_msg, prefix, private_buffer);

        // Send message
        if(send(client_node->client_sockfd, private_buffer, strlen(private_buffer), 0) == -1) {
            stop("error when sending private message");
        }
    }

}

void change_nickname(clientNode* client_head, int client_sockfd, char* new_nickname) {
    /*
    allow client which has client_sockfd in client_list change current nickname to new_nickname
    */
    if(check_nickname_valid(client_head, new_nickname) == 0) {
        //the new nickname is already existed ! send a error message to the client
        char* error_message = (char*)malloc(sizeof(char)*BUFFER_SIZE);
        bzero(error_message, BUFFER_SIZE);
        strcpy(error_message, "sorry this nickname is already used by another user!");
        
        if(write(client_sockfd, error_message, BUFFER_SIZE) == -1) {
            stop("could not send the nickname changing error message to client");
        }
    }else {
        clientNode* client = find_client_by_sockfd(client_head, client_sockfd);
        if(client == NULL) {
            stop("client finding error in change_nickname");
        }
        char* current_nickname = (char*)malloc(sizeof(char) * BUFFER_SIZE);
        strcpy(current_nickname, client->nickname);
        printf("\n%s has changed nickname to %s\n", current_nickname, new_nickname);
        strcpy(client->nickname, new_nickname);
    }
}

clientNode* find_client_by_sockfd(clientNode* client_head, int sockfd) {
    /*
    Find and return a client_node that has a sockfd that is passed in argument
    */
    clientNode* temp = client_head;
    while(temp!=NULL) {
        if(temp->client_sockfd == sockfd) {
            return temp;
        }
        temp = temp->next;
    }
    return NULL;
}

clientNode* find_client_by_nickname(clientNode* client_head, char* nickname) {
    clientNode* temp = client_head;
    while(temp != NULL) {
        if(strcmp(temp->nickname, nickname) == 0) {
            return temp;
        }
        temp = temp->next;
    }
    return NULL;
}

void message_formatted(char* buffer, char* prefix, char* buffer_formatted) {
    /*
    add current_time and nickname at the beginning of buffer and store in buffer_formatted
    */
    remove_enter_in_buffer(buffer);
    char* buffer_offset = (char*)malloc(sizeof(char) * strlen(buffer));
    strncpy(buffer_offset, buffer, strlen(buffer));
    buffer_offset[strlen(buffer)] = '\0';

    char buffer_time[256];
    time_t current_time = time(NULL);
    strftime(buffer_time, sizeof(buffer_time), "%c", localtime(&current_time));
    
    snprintf(buffer_formatted, BUFFER_SIZE, "[%s] %s%s%s",buffer_time, prefix, ": ", buffer_offset);
}

void forward_message(int k, char* buffer, int nBytes) {
    if(write(k, buffer, nBytes) == -1) {
        stop("could not forward message");
    }
}

int remove_enter_in_buffer(char* buffer) {
    /*
    replace '\n' in buffer by '\0'
    Params: buffer
    Return: the len of buffer from beginning to '\0'
    */
    int k;
    for(k = 0; k < BUFFER_SIZE; k++) {
        if(buffer[k] == '\n') {
            buffer[k] = '\0';
            break;
        }
    }
    return k;
}

void clean_worlds_array(char **args) {
    /*
    call remove_enter_in_buffer on every worlds in args
    */
    int i_ = 0;
    char *temp_buf = args[i_];

    while(temp_buf != NULL) {
        remove_enter_in_buffer(temp_buf);
        i_++;
        temp_buf = args[i_];
    }
}

void add_clientNode_to_list(clientNode** client_head ,int client_sockfd, char* nickname_buffer, int nickname_len) {
    /*
    Add clientNode which has nickname_buffer, client_sockfd at the beginning of client_list
    */
    clientNode* node = (clientNode *)malloc(sizeof(clientNode));
    node->client_sockfd = client_sockfd;
    node->nickname = (char *)malloc(sizeof(char) * nickname_len);
    for(int i = 0; i <= nickname_len; i++) {
        node->nickname[i] = nickname_buffer[i];
    }
    if(*client_head == NULL) {
        *client_head = node;
        (*client_head)->next = NULL;
    }else {
        node->next = *client_head;
        *client_head = node;
    }
}

int check_nickname_valid(clientNode* client_head, char*nickname_buffer) {
    /*
    Check whether the nickname_buffer valid or not (the nickname is considered valid if it doesn't existed in 
    client list). If it is return 1 and 0 if it isn't
    */
    clientNode* temp = client_head;
    while(temp != NULL) {
        if(strcmp(nickname_buffer, temp->nickname) == 0) {
            return 0;
        }
        temp = temp->next;
    }
    return 1;
}

void request_nickname(clientNode *client_head, int client_sockfd, char* nickname_buffer, int* nickname_len) {
    /*
    Request the client the nickname untill it's valid and then store it in nickname_buffer
    */
    int n;
    do {
        if ( ( n = read(client_sockfd, nickname_buffer, BUFFER_SIZE) ) == -1){
            stop("could not receive nickname of the client");
        }
        *nickname_len = remove_enter_in_buffer(nickname_buffer);
            
        if(check_nickname_valid(client_head, nickname_buffer) == 0) {
        char warn[] = "invalid nickname";
                //send the warnning message to client
        if(send(client_sockfd, warn, strlen(warn), 0) == -1) {
            stop("could not send warning message to the client");
        }
        }else {
            char greeting_msg[] = "bienvenue";
            if(send(client_sockfd, greeting_msg, strlen(greeting_msg), 0) == -1) {
                stop("could not send greeting message to the client");
            }
        }
    }while(check_nickname_valid(client_head, nickname_buffer) == 0);
}

