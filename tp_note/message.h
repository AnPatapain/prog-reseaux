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

#define MESSAGE_FILE_TYPE "00010101001"
#define BUFFER_SIZE 1024
#define PORT 1234

typedef struct Message {
    int messageType;
    char* body;
}Message;

typedef struct Meta_data {
    char* file_name;
    int size;
}Meta_data;
