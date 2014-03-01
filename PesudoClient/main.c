
#define SOCKET int

// Everything you could possibly need
#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <strings.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>

#include "packets.h"
#include "sockets.h"

int main(int argc, char* argv[])
{
    SOCKET tcp;
    SOCKET udp;

    struct sockaddr_in server;
    struct sockaddr_in client;


    if(argc != 2){
        printf("Usage: %s <server-ip>\n", argv[0]);
        exit(1);
    }

    server.sin_family = AF_INET;
    server.sin_port = 42337;
    server.sin_addr.s_addr = inet_addr(argv[1]);
    client.sin_family = AF_INET;
    client.sin_port = 42337;
    client.sin_addr.s_addr = htonl(INADDR_ANY);

    tcp = socket(AF_INET, SOCK_STREAM, 0);
    udp = socket(AF_INET, SOCK_DGRAM, 0);

    if(bind(tcp, (struct sockaddr*)&client, sizeof(client)) == -1){
        perror("Bind TCP");
        exit(1);
    }

    server.sin_port = 42338;
    client.sin_port = 42338;

    if(bind(udp, (struct sockaddr*)&client, sizeof(client)) == -1){
        perror("Bind UDP");
        exit(1);
    }


    printf("Binding complete.\nPress any key to start handshake\n");
    getchar();

    if(connect(tcp,(struct sockaddr*)&server, sizeof(server)) == -1){
        perror("Connect Failed");
        exit(1);
    }

    return 0;
}
