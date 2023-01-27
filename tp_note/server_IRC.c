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
#include <sys/stat.h>

#define PORT 1234
#define BUFFER_SIZE 1024
#define TRUE 1
#define False 0
#define MAX_CLIENT 3

#define RED "\e[0;31m"
#define YELLOW "\e[0;33m"
#define GREEN "\e[0;32m"
#define BLUE "\e[0;34m"

#define NORMAL_MESSAGE 0
#define ERROR_MESSAGE 1
#define INFORM_MESSAGE 2
#define FILE_MESSAGE 3

typedef struct Message {
    int messageType;
    char* body;
}Message;

typedef struct clientNode {
    int client_sockfd;
    char *nickname;
    struct clientNode* next;
}clientNode;

typedef struct regis_clientNode {
    int client_sockfd;
    char *nickname;
    char* password;
    struct regis_clientNode* next;
}regis_clientNode;

void stop(char *message) {
    perror(message);
    _Exit(1);
}

//Function for server-client connection and communication in chatroom handling
void server_init(int *, struct sockaddr_in* );
void connection_accept(fd_set *readfds, int *fdmax, int master_sockfd, struct sockaddr_in* client_addr, clientNode** client_head, regis_clientNode** regis_client_head);
void chatting(int i, fd_set *readfds, int master_sockfd, int fdmax, clientNode** client_head, regis_clientNode** regis_client_head);
void forward_message(fd_set* readfds, int fdmax, int master_sockfd, int client_sockfd, char* buffer, int nBytes);
void request_nickname(clientNode *client_head, regis_clientNode* regis_client_head, int client_sockfd, char* nickname_buffer, int* nickname_len);
void message_formatted(char* buffer, char* prefix, char* buffer_formatted);
void command_handler(char** args, int client_sockfd, char* nickName, clientNode** client_head, regis_clientNode** regis_client_head, fd_set *readfds, int fdmax, int master_sockfd);
void send_private_message(clientNode* client_head, regis_clientNode* regis_client_head, char* nickName_src, int sockfd_src, char** args);
void client_exit_handling(clientNode** client_head, regis_clientNode** regis_client_head, int client_sockfd, fd_set* readfds);
void change_color_buffer(char* buffer, char* color_code, char* buffer__);
void alerte_broadcast(char* buffer, clientNode* client_head, regis_clientNode* regis_client_head, int client_sockfd, fd_set* readfds, int fdmax, int master_sockfd);
void alerte_personally(char* buffer, clientNode* client_head, regis_clientNode* regis_client_head, int sockfd_src, int sockfd_dst);

// Function for buffer handling
char **split_command(char *commande);
void assembler_args_into_buffer_from_index(char** args, int index, char* buffer);
int remove_enter_in_buffer(char* buffer);
void clean_worlds_array(char **args);

// Function for client_list handling
void add_clientNode_to_list(clientNode**client_head, int client_sockfd, char* nickname_buffer, int nickname_len);
int check_nickname_valid(clientNode* client_head, regis_clientNode* regis_client_head, char*nickname_buffer);
clientNode* find_client_by_sockfd(clientNode* client_head, int sockfd);
clientNode* find_client_by_nickname(clientNode* client_head, char* nickname);
void change_nickname(clientNode* client, regis_clientNode* regis_client_head, int client_sockfd, char* new_nickname);
void remove_node_by_sockfd(clientNode** client_head, int sockfd);
void print_client_list(clientNode*client_head); // FOR TESTING PURPOSE

// Function for regis_client_list handling
void add_regis_clientNode_to_list(regis_clientNode** client_head, int client_sockfd, char* nickname, char* password, int nickname_len, int password_len);
regis_clientNode* find_regis_client_by_sockfd(regis_clientNode* regis_client_head, int sockfd);
regis_clientNode* find_regis_client_by_nickname(regis_clientNode* regis_client_head, char* nickname);
void remove_regis_node_by_sockfd(regis_clientNode** regis_client_head, int sockfd);
void register_client(clientNode** client_head, regis_clientNode** regis_client_head, int client_sockfd, char* currentNickName, char* newNickName, char* password);
void change_nickname_forcely(clientNode *client_head, regis_clientNode *regis_client_head, int client_sockfd, char* password, char* new_nickname);
void unregister_client_handling(clientNode** client_head, regis_clientNode** regis_client_head, regis_clientNode* regis_client);
void send_server_date_to_client(int client_sockfd);

int main() {
    int master_sockfd, fdmax;
    struct sockaddr_in server_addr, client_addr;
    clientNode *client_head = NULL;
    regis_clientNode *regis_client_head = NULL;

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

        // printf("\nnormal client list\n");
        // print_client_list(client_head);


        // printf("\nregis client list\n");
        // regis_clientNode* temp = regis_client_head;
        // while(temp != NULL) {
        //     printf("\n%d %s %s\n", temp->client_sockfd, temp->nickname, temp->password);
        //     temp = temp->next;
        // }

        for (int i = 0; i <= fdmax; i++){
			if (FD_ISSET(i, &actual_readfds)){
				if (i == master_sockfd){
                    // If the master_sockfd is ready it means there is a client want to connect
                    // The readfds, fdmax can be change after this function call
                    connection_accept(&readfds, &fdmax, master_sockfd, &client_addr, &client_head, &regis_client_head);
                }
				else{
                    chatting(i, &readfds, master_sockfd, fdmax, &client_head, &regis_client_head);
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

void connection_accept(fd_set *readfds, int *fdmax, int master_sockfd, struct sockaddr_in* client_addr, clientNode** client_head, regis_clientNode** regis_client_head) {
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
        request_nickname(*client_head, *regis_client_head, client_sockfd, nickname_buffer, &nickname_len);

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

void chatting(int i, fd_set *readfds, int master_sockfd, int fdmax, clientNode** client_head, regis_clientNode** regis_client_head) {
    /*
    Chatting between server and client. There are two big case.
    Case1: the message from client start with / => server must handle the command
    Case2: the message from client is normal message => server must forward it to all clients in readfds except for
    the one that just has sent the message
    */
    
    // Find which client in client list that have sent the message
    char *nickName = (char *)malloc(sizeof(char) * BUFFER_SIZE);
    bzero(nickName, BUFFER_SIZE);
    clientNode* client = find_client_by_sockfd(*client_head, i);
    regis_clientNode* regis_client;
    if(client == NULL) {
        regis_client = find_regis_client_by_sockfd(*regis_client_head, i);
        if(regis_client == NULL) {
            stop("error when finding who send message in two list");
        }
    }
    if(client != NULL) {
        strcpy(nickName, client->nickname);
    }else if(regis_client != NULL) {
        strcpy(nickName, regis_client->nickname);
    }

    char buffer[BUFFER_SIZE];
    bzero(buffer, BUFFER_SIZE);

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
        remove_enter_in_buffer(buffer);
        if(buffer[0] == '/') {
            // printf("\nbuffer in chatting: %s\n", buffer); ->>> BUG HEREEEEEEEEEEEEEE
            char **args = split_command(buffer);
            clean_worlds_array(args);
            //Test whether the args is clean or not
            // The args is not clean before moving into command_handler
            command_handler(args, i, nickName, client_head, regis_client_head, readfds, fdmax, master_sockfd);

        }else {
            char* buffer__ = (char*)malloc(sizeof(char)*BUFFER_SIZE);
            bzero(buffer__, BUFFER_SIZE);
            message_formatted(buffer, nickName, buffer__);
            printf("%s\n",buffer__);

            forward_message(readfds, fdmax, master_sockfd, i, buffer__, strlen(buffer__));
        }

    }
}

void forward_message(fd_set* readfds, int fdmax, int master_sockfd, int client_sockfd, char* buffer, int nBytes) {
    for(int k = 0; k <= fdmax; k++) {
        if(FD_ISSET(k, readfds) && k != client_sockfd && k != master_sockfd) {
            // printf("message will be forwarded: %s from %d to %d\n", buffer, i, k);
            if(write(k, buffer, nBytes) == -1) {
                stop("could not forward message");
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

void command_handler(char** args, int client_sockfd, char* nickName, clientNode** client_head, regis_clientNode** regis_client_head, fd_set *readfds, int fdmax, int master_sockfd) {
    /*
    Handle diverse command by calling the corresponding function based upon the command args
    */
    
    if(strcmp(args[0], "/nickname") == 0 && args[1] != NULL) {
        char* new_nickname = args[1];

        if(args[2] == NULL) {
            // In case normal client want to change nickname
            change_nickname(*client_head, *regis_client_head, client_sockfd, new_nickname);
        }
        else {
            // Precondition: the client must be registered client to do this method
            if(find_regis_client_by_sockfd(*regis_client_head, client_sockfd) != NULL) {
                change_nickname_forcely(*client_head, *regis_client_head, client_sockfd, args[2], new_nickname);
            } else {
                char role_err[] = "sorry you must be registered client to change forcely nickname";
                if(send(client_sockfd, role_err, strlen(role_err), 0) == -1) {
                    stop("send role_err error in command_handler");
                }
            }
        }
    }
    
    else if(strcmp(args[0], "/mp") == 0 && args[1] != NULL && args[2] != NULL) {
        send_private_message(*client_head, *regis_client_head, nickName, client_sockfd, args);
    }
    
    else if(strcmp(args[0], "/exit") == 0) {
        client_exit_handling(client_head, regis_client_head, client_sockfd, readfds);
    }
    
    else if(strcmp(args[0], "/register") == 0 && args[1] != NULL && args[2] != NULL) {
        register_client(client_head, regis_client_head, client_sockfd, nickName, args[1], args[2]);
    }
    
    else if(strcmp(args[0], "/unregister") == 0 && args[1] != NULL && args[2] != NULL) {
        // Precondition: the client must be registered client and the password must be correct to do this method
        regis_clientNode* regis_client = find_regis_client_by_nickname(*regis_client_head, args[1]); 
        if(regis_client != NULL && strcmp(regis_client->password, args[2]) == 0) {
            unregister_client_handling(client_head, regis_client_head, regis_client);    
        } else {
            char role_err[] = "\033[41mThe password is wrong OR you are not registered client\033[m";
            if(send(client_sockfd, role_err, strlen(role_err), 0) == -1) {
                stop("send role_err error in command_handler");
            }
        }
    } 
    
    else if(strcmp(args[0], "/date") == 0) {
        send_server_date_to_client(client_sockfd);
    }
    
    else if(strcmp(args[0], "/alerte") == 0) {
        /*

        PSEUDO_CODE
        Try to find is there a client or regis_client who has a nickname as args[1]
        
        If it is -> alerte broadcast
        If it is not -> alerte personally

        */

        // Find client
        clientNode* client = find_client_by_nickname(*client_head, args[1]);
        regis_clientNode* regis_client = NULL;
        int sockfd_dest;

        if(client == NULL) {
            regis_client = find_regis_client_by_nickname(*regis_client_head, args[1]);
        }
        
        char *buffer = (char*)calloc(sizeof(char), BUFFER_SIZE);

        // If there is any client -> alerte broadcast
        if(client == NULL && regis_client == NULL) {
            assembler_args_into_buffer_from_index(args, 1, buffer);
            alerte_broadcast(buffer, *client_head, *regis_client_head, client_sockfd, readfds, fdmax, master_sockfd);
        }
        // If there is -> alerte personally
        else {
            if(client != NULL) {
                sockfd_dest = client->client_sockfd;
            }else {
                sockfd_dest = regis_client->client_sockfd;
            }
            assembler_args_into_buffer_from_index(args, 2, buffer);
            alerte_personally(buffer, *client_head, *regis_client_head, client_sockfd, sockfd_dest);
        }
    }

    else if(strcmp(args[0], "/send") == 0 && args[1] != NULL && args[2] != NULL) {
        /*
        PSEUDO CODE
        Try to find the client who has speudo as args[1]
        If client is NULL -> send client_not_found to client
        Try to open the file as its name is args[2] with permission read
        If file can not be opened -> send file_not_opened to client

        Send meta data 
        */
        printf("\nstart listenning meta data ...\n");
    }

}

void alerte_broadcast(char* buffer, clientNode* client_head, regis_clientNode* regis_client_head, int client_sockfd, fd_set* readfds, int fdmax, int master_sockfd) {
    char* buffer__ = (char *)calloc(sizeof(char), 1024);
    char* prefix__ = (char *)calloc(sizeof(char), 1024);

    char* prefix = (char *)calloc(sizeof(char), 1024);
    clientNode* client = find_client_by_sockfd(client_head, client_sockfd);
    regis_clientNode* regis_client;
    if(client == NULL) {
        regis_client = find_regis_client_by_sockfd(regis_client_head, client_sockfd);
        snprintf(prefix, BUFFER_SIZE, "%s%s", "alerte from ", regis_client->nickname);
        if(regis_client == NULL) {
            stop("find client error in command handler (alerte)");
        }
    }else {
        snprintf(prefix, BUFFER_SIZE, "%s%s", "alerte from ", client->nickname);
    }

    change_color_buffer(prefix, RED, prefix__);
    message_formatted(buffer, prefix__, buffer__);
    forward_message(readfds, fdmax, master_sockfd, client_sockfd, buffer__, strlen(buffer__));
}

void alerte_personally(char* buffer, clientNode* client_head, regis_clientNode* regis_client_head, int sockfd_src, int sockfd_dst) {
    char* buffer__ = (char *)calloc(sizeof(char), 1024);
    char* prefix__ = (char *)calloc(sizeof(char), 1024);

    char* prefix = (char *)calloc(sizeof(char), 1024);
    clientNode* client = find_client_by_sockfd(client_head, sockfd_src);
    regis_clientNode* regis_client;
    if(client == NULL) {
        regis_client = find_regis_client_by_sockfd(regis_client_head, sockfd_src);
        snprintf(prefix, BUFFER_SIZE, "%s%s", "alerte from ", regis_client->nickname);
        if(regis_client == NULL) {
            stop("find client error in command handler (alerte)");
        }
    }else {
        snprintf(prefix, BUFFER_SIZE, "%s%s", "alerte from ", client->nickname);
    }

    change_color_buffer(prefix, RED, prefix__);
    message_formatted(buffer, prefix__, buffer__);
    
    if(send(sockfd_dst, buffer__, strlen(buffer__), 0) == -1) {
        stop("send alerte to client error in alerte_personally");
    }
}

void change_color_buffer(char* buffer, char* color_code, char* buffer__) {
    /*
    change color message by concatenating color_ascii code and buffer and store the result in buffer__
    */
    char* buffer_offset = (char*)calloc(strlen(buffer), sizeof(char));
    strncpy(buffer_offset, buffer, strlen(buffer));
    buffer_offset[strlen(buffer)] = '\0';
    
    //example: \033[41mThe password is wrong OR you are not registered client\033[m
    // snprintf(buffer__, BUFFER_SIZE, "%s%s%s", color_code, buffer_offset, "e[0;m");
    snprintf(buffer__, BUFFER_SIZE, "%s%s%s", color_code, buffer_offset, "\033[0m");
}

void assembler_args_into_buffer_from_index(char** args, int index, char* buffer) {
    int k = index;
    int j = 0;
    int i = 0;
    while(args[k] != NULL) {
        for(int i = 0; i < strlen(args[k]); i++) {
            buffer[j] = args[k][i];
            j++;
        } 
        buffer[j] = ' ';   
        k++;
        j++;
    }
}

void send_server_date_to_client(int client_sockfd) {
    /*
    Get current time, format it into year-month-hour-minuite-seconde stored in buffer__, then send
    it to client
    */
    char buffer_time[50];
    time_t current_time = time(NULL);
    strftime(buffer_time, sizeof(buffer_time), "%Y-%m-%d %H:%M:%S", localtime(&current_time));

    char* buffer__ = (char*)calloc(sizeof(char), BUFFER_SIZE);

    change_color_buffer(buffer_time, GREEN, buffer__);

    if(send(client_sockfd, buffer__, strlen(buffer__), 0) == -1) {
        stop("send server time to client error in send_server_date_to_client");
    }

}


void unregister_client_handling(clientNode** client_head, regis_clientNode** regis_client_head, regis_clientNode* regis_client) {
    /*
    remove client in regis_client_list and add client into normal client list
    */
    int regis_client_sockfd = regis_client->client_sockfd;
    char nickname_buffer[BUFFER_SIZE];
    bzero(nickname_buffer, BUFFER_SIZE);

    for(int i = 0; i < BUFFER_SIZE; i++) {
        nickname_buffer[i] = regis_client->nickname[i];
    }

    add_clientNode_to_list(client_head, regis_client->client_sockfd, nickname_buffer, strlen(nickname_buffer));
    remove_regis_node_by_sockfd(regis_client_head, regis_client->client_sockfd);

    // Send msg inform to client
    char unregister_msg[] = "Change role successfully. You are normal client now";
    if(send(regis_client_sockfd, unregister_msg, strlen(unregister_msg), 0) == -1) {
        stop("send unregister_msg error in unregister_client_handling");
    }
}

void client_exit_handling(clientNode** client_head, regis_clientNode** regis_client_head, int client_sockfd, fd_set* reafds) {
    // Check the client is in normal list or regis_client_list
    clientNode* client = find_client_by_sockfd(*client_head, client_sockfd);
    regis_clientNode* regis_client = find_regis_client_by_sockfd(*regis_client_head, client_sockfd);
    if(client == NULL && regis_client == NULL) {
        stop("finding client error in exit_handling in server side");
    }

    // remove client from list chainee
    if(client != NULL) {
        remove_node_by_sockfd(client_head, client_sockfd);
    }else {
        remove_regis_node_by_sockfd(regis_client_head, client_sockfd);
    }
    
    // clear sockfd of it from readfds
    close(client_sockfd);
    FD_CLR(client_sockfd, reafds);
}

void remove_node_by_sockfd(clientNode** client_head, int client_sockfd) {
    // Store head node
    clientNode* temp = *client_head;
    clientNode* prev;

    // If head node itself holds the key to be deleted
    if (temp != NULL && temp->client_sockfd == client_sockfd) {
        *client_head = temp->next;   // Changed head
        free(temp);               // free old head
        return;
    }

    // Search for the client_sockfd to be deleted, keep track of the
    // previous node as we need to change 'prev->next'
    while (temp != NULL && temp->client_sockfd != client_sockfd) {
        prev = temp;
        temp = temp->next;
    }

    // If client_sockfd was not present in linked list
    if (temp == NULL) return;

    // Unlink the node from linked list
    prev->next = temp->next;

    free(temp);  // Free memory
}

void remove_regis_node_by_sockfd(regis_clientNode** regis_client_head, int client_sockfd) {
    regis_clientNode* temp = *regis_client_head;
    regis_clientNode* prev;

    // If head node itself holds the key to be deleted
    if (temp != NULL && temp->client_sockfd == client_sockfd) {
        *regis_client_head = temp->next;   // Changed head
        free(temp);               // free old head
        return;
    }

    // Search for the client_sockfd to be deleted, keep track of the
    // previous node as we need to change 'prev->next'
    while (temp != NULL && temp->client_sockfd != client_sockfd) {
        prev = temp;
        temp = temp->next;
    }

    // If client_sockfd was not present in linked list
    if (temp == NULL) return;

    // Unlink the node from linked list
    prev->next = temp->next;

    free(temp);  // Free memory
}

void send_private_message(clientNode* client_head, regis_clientNode* regis_client_head, char* nickName_src, int sockfd_src, char** args){
    clientNode* client_node = find_client_by_nickname(client_head, args[1]);
    regis_clientNode* regis_client_node = find_regis_client_by_nickname(regis_client_head, args[1]);

    if(client_node == NULL && regis_client_node == NULL) {
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
        sprintf(prefix, "%s%s", "private msg from ", nickName_src);

        char* private_buffer = (char*)malloc(sizeof(char) * BUFFER_SIZE);
        bzero(private_buffer, BUFFER_SIZE);

        message_formatted(private_msg, prefix, private_buffer);

        int target_sockfd;
        if(client_node != NULL) {
            target_sockfd = client_node->client_sockfd;
        }else {
            target_sockfd = regis_client_node->client_sockfd;
        }
        // Send message
        if(send(target_sockfd, private_buffer, strlen(private_buffer), 0) == -1) {
            stop("error when sending private message");
        }
    }

}

void change_nickname(clientNode* client_head, regis_clientNode* regis_client_head, int client_sockfd, char* new_nickname) {
    /*
    allow client which has client_sockfd in client_list change current nickname to new_nickname
    */
    if(check_nickname_valid(client_head, regis_client_head, new_nickname) == 0) {
        //the new nickname is already existed ! send a error message to the client
        char* error_message = (char*)malloc(sizeof(char)*BUFFER_SIZE);
        bzero(error_message, BUFFER_SIZE);
        strcpy(error_message, "sorry this nickname is already used by another user!");
        
        if(write(client_sockfd, error_message, BUFFER_SIZE) == -1) {
            stop("could not send the nickname changing error message to client");
        }
    }else {
        clientNode* client = find_client_by_sockfd(client_head, client_sockfd);
        regis_clientNode* regis_client;
        
        if(client == NULL) {
            regis_client = find_regis_client_by_sockfd(regis_client_head, client_sockfd);
            if(regis_client == NULL){
                stop("find client fail in change_nickname");
            }    
        }
        char* current_nickname = (char*)calloc(BUFFER_SIZE, sizeof(char));
        
        if(client != NULL) {
            strcpy(current_nickname, client->nickname);
            strcpy(client->nickname, new_nickname);
        }
        else if(regis_client != NULL) {
            strcpy(current_nickname, regis_client->nickname);
            strcpy(regis_client->nickname, new_nickname);
        }
        printf("\n%s has changed nickname to %s\n", current_nickname, new_nickname);
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

regis_clientNode* find_regis_client_by_sockfd(regis_clientNode* regis_client_head, int sockfd) {
    regis_clientNode* temp = regis_client_head;
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

regis_clientNode* find_regis_client_by_nickname(regis_clientNode* regis_client_head, char* nickname) {
    regis_clientNode* temp = regis_client_head;
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
    char* buffer_offset = (char*)calloc(strlen(buffer), sizeof(char));
    strncpy(buffer_offset, buffer, strlen(buffer));
    buffer_offset[strlen(buffer)] = '\0';
    
    // char buffer_time[50];
    // time_t current_time = time(NULL);
    // printf("\ninner flag 4\n");
    // printf("\n%d\n", current_time);
    // strftime(buffer_time, sizeof(buffer_time), "%Y-%m-%d %H:%M:%S", localtime(&current_time));
    // printf("\ninner flag 5\n");

    // snprintf(buffer_formatted, BUFFER_SIZE, "[%s] %s%s%s",buffer_time, prefix, ": ", buffer_offset);
    snprintf(buffer_formatted, BUFFER_SIZE, "%s%s%s", prefix, ": ", buffer_offset);
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
    If the client_list was not created then create it
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

void add_regis_clientNode_to_list(regis_clientNode** regis_client_head, int client_sockfd, char* nickname, char* password, int nickname_len, int password_len) {
    regis_clientNode* node = (regis_clientNode *)malloc(sizeof(regis_clientNode));
    node->client_sockfd = client_sockfd;
    node->nickname = (char *)calloc(1024, sizeof(char));
    for(int i = 0; i <= nickname_len; i++) {
        node->nickname[i] = nickname[i];
    }
    node->password = (char *)calloc(1024, sizeof(char));
    for(int i = 0; i <= password_len; i++) {
        node->password[i] = password[i];
    }
    if(*regis_client_head == NULL) {
        *regis_client_head = node;
        (*regis_client_head)->next = NULL;
    }else {
        node->next = *regis_client_head;
        *regis_client_head = node;
    }
}

int check_nickname_valid(clientNode* client_head, regis_clientNode* regis_client_head, char*nickname_buffer) {
    /*
    Check whether the nickname_buffer valid or not (the nickname is considered valid if it doesn't existed in 
    client list and regis_client list). If it is return 1 and 0 if it isn't
    */
    
    clientNode* client = find_client_by_nickname(client_head, nickname_buffer);
    regis_clientNode* regis_client = find_regis_client_by_nickname(regis_client_head, nickname_buffer);
    
    if(client != NULL || regis_client != NULL) {
        return 0;
    }
    return 1;
}

void request_nickname(clientNode *client_head, regis_clientNode* regis_client_head, int client_sockfd, char* nickname_buffer, int* nickname_len) {
    /*
    Request the client the nickname untill it's valid and then store it in nickname_buffer
    */
    int n;
    do {
        if ( ( n = read(client_sockfd, nickname_buffer, BUFFER_SIZE) ) == -1){
            stop("could not receive nickname of the client");
        }
        *nickname_len = remove_enter_in_buffer(nickname_buffer);
            
        if(check_nickname_valid(client_head, regis_client_head, nickname_buffer) == 0) {
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
    }while(check_nickname_valid(client_head, regis_client_head, nickname_buffer) == 0);
}

void register_client(clientNode** client_head, regis_clientNode** regis_client_head, int client_sockfd, char* currentNickName, char* newNickName, char* password) {
    int create_successfully = 0;
    
    if(find_regis_client_by_sockfd(*regis_client_head, client_sockfd) != NULL) {
        char warn_[] = "sorry you has already registered";
        if(send(client_sockfd, warn_, strlen(warn_), 0) == -1) {
            stop("error when sending warn_ in command handler");
        }
    }else {
        if(strcmp(currentNickName, newNickName) == 0) {
            
            //TODO: add node to regis_client_list
            //Metadata for regis_node: client_sockfd, nickname, password, nickname_len, password_len
            add_regis_clientNode_to_list(regis_client_head, client_sockfd, newNickName, password, strlen(newNickName), strlen(password));
            create_successfully = 1;
            
        }else {
            //TODO: Check if there is a non registered client are actually using this args[1] as nickname
            // If it is then refuse to create new regis client and exit current activity
            // If it is not then create new regis client and set create_successfully to 1
            clientNode* temp_client = find_client_by_nickname(*client_head, newNickName);
            regis_clientNode* temp_regis_client = find_regis_client_by_nickname(*regis_client_head, newNickName);
            char warn_buffer[] = "sorry this nickname is existed";
            if(temp_client != NULL || temp_regis_client != NULL) {
                if(send(client_sockfd, warn_buffer, strlen(warn_buffer), 0) == -1) {
                    stop("error when send warn buffer in register case in command handler");
                }
            }else {
                add_regis_clientNode_to_list(regis_client_head, client_sockfd, newNickName, password, strlen(newNickName), strlen(password));
                create_successfully = 1;
            }

        }

        if (create_successfully) {
            //TODO: remove the user in normal client_list
            remove_node_by_sockfd(client_head, client_sockfd);
        }
    }
}

void change_nickname_forcely(clientNode *client_head, regis_clientNode *regis_client_head, int client_sockfd, char* password, char* new_nickname) {
    /*
    Forcely change nickname by requesting the normal client which currently hold the newnickname to change its nickname
    */

    // PSEUDO Code
    
    // client_head -> [thu] -> [ngoc_ngan] -> NULL
    // regis_client_head -> [an] -> NULL
            
    // Find register client by nickname

    // Verify password
            
    // If it's not -> print password error to register user -> exit flow
            
    // If it is
        // send waiting message to client
        // find normal client which has new_nickname
            
        // If there is no such client -> just simply change -> exit flow 
        // If there is
            // Server sends a message to normal client to request it change to other nickname
            // Server receive nickname and request normal client until this nickname valide
            // change nickname for normal client
            // change nickname for register client

            // Inform to normal client and regis client their new nickname
            // exit flow

    regis_clientNode* regis_client = find_regis_client_by_sockfd(regis_client_head, client_sockfd);
    
    if (regis_client == NULL) {
        stop("find regis_client error in change_nickname_forcely");
    }

    if(strcmp(password, regis_client->password) != 0) {
        char pass_err[] = "Wrong password";
        if(send(client_sockfd, pass_err, strlen(pass_err), 0) == -1) {
            stop("send pass_err error in change_nickname_forcely");
        }
    }
    else {
        char wait_msg[] = "Please wait for the server request normal client changing its nickname";
        if(send(client_sockfd, wait_msg, strlen(wait_msg), 0) == -1) {
            stop("send wait_msg error in change_nickname_forcely");
        }

        clientNode* normal_client = find_client_by_nickname(client_head, new_nickname);


        char change_success_msg[] = "change nickname successfully";
        if(normal_client == NULL) {
            char* current_nickname = (char*)calloc(BUFFER_SIZE, sizeof(char));
    
            strcpy(current_nickname, regis_client->nickname);
            strcpy(regis_client->nickname, new_nickname);
            
            
            if(send(client_sockfd, change_success_msg, strlen(change_success_msg), 0) == -1) {
                stop("send change_success_msg error in change_nickname_forcely");
            }
            printf("\n%s has changed nickname to %s\n", current_nickname, new_nickname);
        }else {
            char change_nickname_rq[] = "Please change your nickname Please Please Please";
            
            if(send(normal_client->client_sockfd, change_nickname_rq, strlen(change_nickname_rq), 0) == -1 ) {
                stop("send change_nickname_rq error in change_nickname_forcely");
            }

            char nickname_buffer[BUFFER_SIZE];
            int nickname_len;
            bzero(nickname_buffer, BUFFER_SIZE);
            request_nickname(client_head, regis_client_head, normal_client->client_sockfd, nickname_buffer, &nickname_len);
            
            // change nickname of normal client to nickname_buffer
            normal_client->nickname = (char*)calloc(sizeof(char), BUFFER_SIZE);
            for(int i = 0; i < BUFFER_SIZE; i++) {
                normal_client->nickname[i] = nickname_buffer[i];
            }
            
            if(send(normal_client->client_sockfd, change_success_msg, strlen(change_success_msg), 0) == -1) {
                stop("send change_success_msg error in change_nickname_forcely");
            }
            
            // change nickname of regis client to new_nickname
            strcpy(regis_client->nickname, new_nickname);
            
            if(send(client_sockfd, change_success_msg, strlen(change_success_msg), 0) == -1) {
                stop("send change_success_msg error in change_nickname_forcely");
            }

        }
        
    }

}

void print_client_list(clientNode* client_head) {
    clientNode* temp = client_head;
    while(temp != NULL) {
        printf("\n%d %s\n", temp->client_sockfd, temp->nickname);
        temp = temp->next;
    }
}
