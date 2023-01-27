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
#include <sys/stat.h>

#define PORT 1234
#define BUFFER_SIZE 1024

#define CLIENT_READY 0

typedef struct Meta_data {
    char* file_name;
    int size;
}Meta_data;

void stop(char*);

void connect_to_server(int *sockfd, struct sockaddr_in* server_addr);

void chatting(int i, int sockfd);

char **split_command(char *commande);

int remove_enter_in_buffer(char* buffer);

void clean_worlds_array(char **args);

void client_command_handler(char** args, int sockfd);

void send_nickname_to_server(int sockfd);

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
        //If connect successfully then try to send a unique nickname to the server
        send_nickname_to_server(*sockfd);
    }
}

void send_nickname_to_server(int sockfd) {
    char server_msg[BUFFER_SIZE];
    do {
        char nickname_buf[BUFFER_SIZE];
        printf("write your nickname: ");
        fgets(nickname_buf, BUFFER_SIZE, stdin);
        // Send the nickname of client to server
        if(send(sockfd, nickname_buf, BUFFER_SIZE, 0) == -1) {
            stop("error when sending the nickname of client to the server");
        }

        bzero(server_msg, BUFFER_SIZE);
        if(recv(sockfd, server_msg, BUFFER_SIZE, 0) == -1) {
            stop("could not receive confirm message from the server");
        }
        printf("server reply: %s\n", server_msg);
    }while(strcmp(server_msg, "bienvenue") != 0);
}

void chatting(int i, int sockfd)
{
	char send_buf[BUFFER_SIZE];
	char recv_buf[BUFFER_SIZE];
	int nbyte_recvd;
	
	if (i == 0){
		fgets(send_buf, BUFFER_SIZE, stdin);
        write(sockfd, send_buf, strlen(send_buf));

        remove_enter_in_buffer(send_buf);
        char** args = split_command(send_buf);
        clean_worlds_array(args);

        if(send_buf[0] == '/' && strcmp(args[0], "/send") == 0){    
            client_command_handler(args, sockfd);
        }else {
            if(strcmp(send_buf, "/exit\n") == 0) {
                exit(0);
            }
        }
        
	}else {
		nbyte_recvd = recv(sockfd, recv_buf, BUFFER_SIZE, 0);
		recv_buf[nbyte_recvd] = '\0';
		printf("%s\n" , recv_buf);
		fflush(stdout);
	}
}

void client_command_handler(char** args, int sockfd) {
    /*
    This function for some speicial command that need to be handled in client side. Ex: send file command
    */
    if(strcmp(args[0], "/send") == 0 && args[1] != NULL && args[2] != NULL) {
        // Open file
        FILE* file;
        if((file = fopen(args[2], "r")) == NULL){
            printf("\nfile doesn't exist\n");
        }else{
            printf("\nopen file successfully start sending meta data to server\n");
            struct stat stat_buf;
            stat("text_src.txt", &stat_buf);
            Meta_data* meta_data = (Meta_data*)malloc(sizeof(meta_data));
            meta_data->size = stat_buf.st_size;
            meta_data->file_name = (char*)calloc(sizeof(char), 1024);
            strncpy(meta_data->file_name, args[2], strlen(args[2]));

            if(send(sockfd, meta_data, sizeof(meta_data), 0) == -1) {
                stop("send meta_data error in client_command_handler");
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

void stop(char *message) {
    perror(message);
    _Exit(1);
}
