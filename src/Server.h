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
#ifndef SERVER
#define SERVER

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

#include "NetComm.h"

#define NUM_IPC_PACKETS 5

#define SERVER_VERSION 1.2

#define TCP_PORT 42337
#define UDP_PORT 42338

// **********************************
//             DEBUG
// **********************************
#define DEBUG_ON 1
#define DEBUG_INFO  1
#define DEBUG_WARN  2
#define DEBUG_ALRM  3
#define DEBUG_LEVEL 2
#define DEBUG(lvl, msg) if(DEBUG_ON && lvl >= DEBUG_LEVEL){printf("Debug: %s\n", msg);fflush(stdout);}

#define TRUE    1
#define FALSE   PLAYER_STATE_INVALID


// **********************************
//           MOVE UPDATES
// **********************************
#define MOVE_UPDATE_FREQUENCY 0.5 // Updates per seoncd

typedef int     SOCKET;

//function prototypes for server-utils
packet_t    getPacketType(SOCKET socket);
int         getPacket(SOCKET socket, void* buffer, int sizeOfBuffer);

//function prototypes for game-utils
pos_t       getLobbyX(playerNo_t plyr);
pos_t       getLobbyY(playerNo_t plyr);

void* ConnectionManager(void* ipcSocks);
void* InboundSwitchboard(void* ipcSocks);
void* GameplayController(void* ipcSocks);
void* GeneralController(void* ipcSocks);
void* UIController(void* ipcSocks);
void* OutboundSwitchboard(void* ipcSocks);
void* KeepAlive(void* outSock);
void* MovementTimer(void* ipcSocks);

// structures
typedef struct pktB0{
	char				serverName[MAX_NAME];
	int					maxPlayers;
} PKT_SERVER_SETUP;

#define IPC_PKT_0 0xB0

typedef struct pktB1{
	playerNo_t			playerNo;
	character_t         character;
	char 				client_player_name[MAX_NAME];
} PKT_NEW_CLIENT;

#define IPC_PKT_1 0xB1

typedef struct pktB2{
	playerNo_t			playerNo;
} PKT_LOST_CLIENT;

#define IPC_PKT_2 0xB2

typedef struct pktB3{
    playerNo_t          playerNo;
    floorNo_t           newFloor;
} PKT_FORCE_MOVE;

#define IPC_PKT_3 0xB3

// Packet B4 is alarm packet
#define IPC_PKT_4 0xB4

// **********************************
//           OUT MASK
// **********************************
#define OUTMASK int_fast32_t
#define OUT_SET(mask, pos) mask|=(1<<(pos))
#define OUT_SETALL(mask) mask=0xFFFFFFFF
#define OUT_ZERO(mask) mask=0x00000000
#define OUT_ISSET(mask, pos) ((mask&(1<<(pos)))==(1<<(pos)))

// global data stores
SOCKET 				tcpConnections[MAX_PLAYERS];
SOCKET				udpConnection;
struct sockaddr_in 	udpAddresses[MAX_PLAYERS];
int                 connectedPlayers;

int netPacketSizes[NUM_NET_PACKETS + 1];
int ipcPacketSizes[NUM_IPC_PACKETS + 1];
int largestNetPacket, largestIpcPacket, largestPacket;


// **********************************
//            KEEP ALIVE
// **********************************
#define CHECK_CONNECTIONS 0
#define CLEANUP_FREQUENCY 5
#define PRESUME_DEAD_FREQUENCY 15
time_t clientHeartbeat[MAX_PLAYERS];
time_t serverHeartbeat[MAX_PLAYERS];
void clientPulse(playerNo_t plyr);
void serverPulse(playerNo_t plyr);


void chatGameStart();
void sendChat(PKT_CHAT* chat, teamNo_t teams[MAX_PLAYERS], SOCKET outswitch);
#endif
