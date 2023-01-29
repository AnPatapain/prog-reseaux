CC = gcc
CFLAGS = -Wall -g
SRC_DIR = src

all: client_IRC server_IRC tcpserver tcpclient

client_IRC: $(SRC_DIR)/client_IRC.o 
	$(CC) $(CFLAGS) -o client_IRC $(SRC_DIR)/client_IRC.o 

server_IRC: $(SRC_DIR)/server_IRC.o 
	$(CC) $(CFLAGS) -o server_IRC $(SRC_DIR)/server_IRC.o 

tcpclient: $(SRC_DIR)/tcpclient.o
	$(CC) $(CFLAGS) -o tcpclient $(SRC_DIR)/tcpclient.o

tcpserver: $(SRC_DIR)/tcpserver.o
	$(CC) $(CFLAGS) -o tcpserver $(SRC_DIR)/tcpserver.o

$(SRC_DIR)/client_IRC.o: $(SRC_DIR)/client_IRC.c $(SRC_DIR)/message.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/client_IRC.c -o $(SRC_DIR)/client_IRC.o

$(SRC_DIR)/server_IRC.o: $(SRC_DIR)/server_IRC.c $(SRC_DIR)/message.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/server_IRC.c -o $(SRC_DIR)/server_IRC.o

$(SRC_DIR)/tcpclient.o: $(SRC_DIR)/tcpclient.c
	$(CC) $(CFLAGS) -c $(SRC_DIR)/tcpclient.c -o $(SRC_DIR)/tcpclient.o

$(SRC_DIR)/tcpserver.o: $(SRC_DIR)/tcpserver.c
	$(CC) $(CFLAGS) -c $(SRC_DIR)/tcpserver.c -o $(SRC_DIR)/tcpserver.o

clean:
	rm -f client_IRC server_IRC tcpclient tcpserver $(SRC_DIR)/*.o
