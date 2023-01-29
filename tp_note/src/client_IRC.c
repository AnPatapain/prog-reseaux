#include "message.h"

#define CLIENT_READY 0

void stop(char*);

void connect_to_server(int *sockfd, struct sockaddr_in* server_addr);

void chatting(int i, int sockfd);

char **split_command(char *commande);

int remove_enter_in_buffer(char* buffer);

void clean_worlds_array(char **args);

void send_file_handler(char** args, int sockfd);

void receive_file_handler(int sockfd);

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
    bzero(recv_buf, BUFFER_SIZE);
	int nbyte_recvd;
	
	if (i == 0){
        // SEND MODE. The client activated -> Read message -> Send to server
		fgets(send_buf, BUFFER_SIZE, stdin);
        write(sockfd, send_buf, strlen(send_buf));

        remove_enter_in_buffer(send_buf);
        char** args = split_command(send_buf);
        clean_worlds_array(args);

        if(strcmp(args[0], "/send") == 0 && args[1] != NULL && args[2] != NULL){ 
            send_file_handler(args, sockfd);
        }else if( strncmp(args[0], "/exit", 5) == 0) {
            exit(0);
        }
        
	}else {
        // RECEIVE MODE. The server activate -> Receive message then handle it
        // Receive MESSAGE_TYPE if the TYPE is FILE -> Handle. If not -> print it
        if( (nbyte_recvd = recv(sockfd, recv_buf, BUFFER_SIZE, 0)) == -1 ) {
            stop("receive inform message type error in client");
        }

        if(nbyte_recvd == 0) {
            printf("\n\e[0;33mThe server has been closed. Ciao\033[0m\n");
            exit(1);
        }

        if(nbyte_recvd < 0) {
            stop("error when receive message from server in client side");
        }  
		

        if(strcmp(recv_buf, MESSAGE_FILE_TYPE) == 0) {
            
            receive_file_handler(sockfd);

        }else {

            printf("%s\n" , recv_buf);
        
        }
		fflush(stdout);
	}
}

void receive_file_handler(int sockfd) {
    // receive Meta Data from server
    int size;
    char* file_name_buffer = (char*)calloc(sizeof(char), BUFFER_SIZE);
        
    if(recv(sockfd, &size, sizeof(int), 0) == -1) {
        stop("receive meta data size error client side");
    }

    if(recv(sockfd, file_name_buffer, BUFFER_SIZE, 0) == -1) {
        stop("receive meta data file name error client side");
    }

    printf("\n\e[0;32mmeta data received: %s, %d\033[0m\n",file_name_buffer, size);

    // Use meta data to create a file in current directory
    char *file_name_dst = (char*)calloc(sizeof(char), BUFFER_SIZE);
    char *dot = strrchr(file_name_buffer, '.');
    int len = dot - file_name_buffer;
    sprintf(file_name_dst, "%.*s_dst.txt", len, file_name_buffer);

    FILE *fp = fopen("text_dst.txt", "w");
    if (fp == NULL) {
        stop("Error opening file");
    }

    // send ready message to server
    char ready_to_receive_data_chunk[] = "ready";
    if(send(sockfd, ready_to_receive_data_chunk, BUFFER_SIZE, 0) == -1) {
        stop("send ready error in recipient");
    }

    // Start receiving data chunk from server and write to file
    char *data_chunk = (char *) malloc(BUFFER_SIZE);
    int bytes_received = recv(sockfd, data_chunk, size, 0);
    if(bytes_received == -1) {
        stop("receiving data chunk error in client");
    }
    printf("\n+++++++++++++++++%s+++++++++++++++++++++\n", file_name_dst);
    printf("\n%s\n", data_chunk);
    printf("\n+++++++++++++++++++++++++++++++++++++++++\n");
            
    int bytes_written = fwrite(data_chunk, 1, bytes_received, fp);
    if(bytes_written != bytes_received) {
        stop("error during writting to file dst");
    }

    printf("\n\e[0;32m Le fichier a ete enregistre dans votre repertoire courante %s\033[0m\n", file_name_dst);
    printf("\nexit file send-recv flow\n");

    // // Free the memory for the data chunk
    free(data_chunk);

    // // Close the file
    fclose(fp);
}

void send_file_handler(char** args, int sockfd) {
    /*
    This function for some speicial command that need to be handled in client side. Ex: send file command
    */
    // Open file
    FILE* file;
    char open_success[] = "ok";
    char open_failure[] = "not ok";
    
    if((file = fopen(args[2], "r")) == NULL){
        printf("\n\e[0;31mfile doesn't exist\033[0m\n");
        if(send(sockfd, open_failure, BUFFER_SIZE, 0) == -1) {
            stop("send open_failure error in client_command handler");
        }
    }
    
    else{
        if(send(sockfd, open_success, BUFFER_SIZE, 0) == -1) {
            stop("send open_success error in client_command handler");
        }
        printf("\n\e[0;32mopen file successfully start sending meta data to server\033[0m\n");
        struct stat stat_buf;
        stat(args[2], &stat_buf);

        // Send meta data to server
        int size = stat_buf.st_size;

        if(send(sockfd, &size, sizeof(int), 0) == -1) {
            stop("send meta_data size error in send_file_handler");
        }

        printf("\nargs[2] before send to server: %s\n", args[2]);
        if(send(sockfd, args[2], strlen(args[2]), 0) == -1) {
            stop("send meta_data file name error in send_file_handler");
        }

        // receive a ready message of server
        char ready[BUFFER_SIZE];
        bzero(ready, BUFFER_SIZE);
        if(recv(sockfd, ready, BUFFER_SIZE, 0) == -1) {
            stop("receive ready error in client side");
        }

        if(strncmp(ready, "ready", 5) == 0) {
            printf("\n\e[0;32mstart sending data file chunk\033[0m\n");
            // Start reading file and send to server data chunk
            char *data_chunk = (char *) malloc(BUFFER_SIZE);
                
            int bytes_read = fread(data_chunk, 1, BUFFER_SIZE, file);
            if(send(sockfd, data_chunk, bytes_read, 0) == -1) {
                stop("sending data chunk error in client");
            }
            printf("\n\e[0;32mSuccessfully sending data file chunk\033[0m\n");
            printf("\nexit file send-recv flow\n");
            free(data_chunk);

            // Close the file
            fclose(file);
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
