/*---------------------------------------------------------------------* 
-- HEADER FILE: Server.h 		The definitions and declarations to be
-- 								to be used to communicate between the
-- 								components of the Server
--
-- PROGRAM:		Game-Server
--
-- FUNCTIONS:
-- 		none
--
-- DATE: 		January 27, 2014
--
-- REVISIONS: 	none
--
-- DESIGNER: 	Andrew Burian
--
-- PROGRAMMER: 	Andrew Burian
--
-- NOTES:
-- This is unique to the server
----------------------------------------------------------------------*/

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

#include "NetComm.h"
#include "Sockets.h"

#define NUM_IPC_PACKETS 3

#define SERVER_VERSION 0.2

#define TCP_PORT 42337
#define UDP_PORT 42338

#define DEBUG_ON 1
#define DEBUG(msg) if(DEBUG_ON){printf("Debug: %s\n", msg);}

//function prototypes
void* ConnectionManager(void* ipcSocks);
void* InboundSwitchboard(void* ipcSocks);
void* GameplayController(void* ipcSocks);
void* GeneralController(void* ipcSocks);
void* UIController(void* ipcSocks);
void* OutboundSwitchboard(void* ipcSocks);

// structures
typedef struct pktB0{
	char				serverName[MAX_NAME];
	int					maxPlayers;
} PKT_SERVER_SETUP;

#define IPC_PKT_0 0xB0

typedef struct pktB1{
	playerNo_t			playerNo;
	char 				client_player_name[MAX_NAME];
} PKT_NEW_CLIENT;

#define IPC_PKT_1 0xB1

typedef struct pktB2{
	playerNo_t			playerNo;
} PKT_LOST_CLIENT;

#define IPC_PKT_2 0xB2



// global data stores

SOCKET 				tcpConnections[MAX_PLAYERS];
SOCKET				udpConnection;
struct sockaddr_in 	udpAddresses[MAX_PLAYERS];


int netPacketSizes[NUM_NET_PACKETS + 1];
int ipcPacketSizes[NUM_IPC_PACKETS + 1];
int largestNetPacket, largestIpcPacket, largestPacket;

