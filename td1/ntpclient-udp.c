#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

// 00.00.00.00 
// 00.00.00.11

// Get li vn or mode as params and set the li vn mode correponding of packet
// For SET_LI we want to set two bits at beginning so we do right shift 6 bit on li (params) and do or bit to bit with packet.li_vn_mode to set its value

#define SET_LI( packet , li )( uint8_t )( packet.li_vn_mode |= ( li << 6 ) )
#define SET_VN( packet , vn )( uint8_t )( packet.li_vn_mode |= ( vn << 3 ) )
#define SET_MODE( packet , mode )( uint8_t )( packet.li_vn_mode |= ( mode << 0 ) )

typedef struct
{
    uint8_t li_vn_mode;      // Eight bits. li, vn, and mode.
                           // li.   Two bits.   Leap indicator.
                           // vn.   Three bits. Version number of the protocol.
                           // mode. Three bits. Client will pick mode 3 for client.

    uint8_t stratum;         // Eight bits. Stratum level of the local clock.
    uint8_t poll;            // Eight bits. Maximum interval between successive messages.
    uint8_t precision;       // Eight bits. Precision of the local clock.

    uint32_t rootDelay;      // 32 bits. Total round trip delay time.
    uint32_t rootDispersion; // 32 bits. Max error aloud from primary clock source.
    uint32_t refId;          // 32 bits. Reference clock identifier.

    uint32_t refTm_s;        // 32 bits. Reference time-stamp seconds.
    uint32_t refTm_f;        // 32 bits. Reference time-stamp fraction of a second.

    uint32_t origTm_s;       // 32 bits. Originate time-stamp seconds.
    uint32_t origTm_f;       // 32 bits. Originate time-stamp fraction of a second.

    uint32_t rxTm_s;         // 32 bits. Received time-stamp seconds.
    uint32_t rxTm_f;         // 32 bits. Received time-stamp fraction of a second.

    uint32_t txTm_s;         // 32 bits and the most important field the client cares about. Transmit time-stamp seconds.
    uint32_t txTm_f;         // 32 bits. Transmit time-stamp fraction of a second.

} ntp_packet;              // Total: 384 bits or 48 bytes.

void error(char *message) {
    perror(message);
    _Exit(0);
}

int main() {
    ntp_packet packet = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    SET_LI(packet, 0);
    SET_VN(packet, 3);
    SET_MODE(packet, 3);
    
    //li_vn_mode = 27(10) = 00011011(2)

    struct hostent* server;
    struct sockaddr_in serv_addr;
    int portno = 123;
    int n;// number byte read or write in or from packet

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    //create udp socket
    if(sockfd == -1) {
        error("could not create socket");
    }

    //Find the server
    char host_name[1024] = "fr.pool.ntp.org";
    server = gethostbyname(host_name);

    if(server == NULL) {
        error("Server not found");
    }

    //initialize zero for server address structure
    bzero(&serv_addr, sizeof(serv_addr));
    
    //communicate with IP4
    serv_addr.sin_family = AF_INET;

    //copy server address to serv_addr
    bcopy(server->h_addr, &serv_addr.sin_addr.s_addr, server->h_length);

    //communicate with client through port 123
    serv_addr.sin_port = htons(portno);


    if(connect(sockfd, (struct sockaddr*)(&serv_addr), sizeof(serv_addr)) == -1) {
        error("can not etablish a connection");
    }

    n = write(sockfd, (char*)(&packet), sizeof(ntp_packet));
    if(n < 0) {
        error("error occur when write to packet");
    }

    n = read(sockfd, (char*)(&packet), sizeof(ntp_packet));
    if(n < 0) {
        error("error occur when read from packet");
    }

    packet.txTm_s = ntohl(packet.txTm_s); 
    time_t txTm = ( time_t ) ( packet.txTm_s - 2208988800);
    printf( "Time: %s", ctime( ( const time_t* ) &txTm ) );
    return 0;
}