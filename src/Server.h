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


// ************************************************************************************************
//                                         CONTROL PANEL
// ************************************************************************************************

    // Version no
    #define SERVER_VERSION          3.7

    // Net comm control
    #define TCP_PORT                42337
    #define UDP_PORT                42338

    // Presets
    #define RUN_AT_LIMIT            1       // Default to running the server at MAX_PLAYERS

    // Debug control
    #define DEBUG_ON                0       // Toggle all debug statements on or off
    #define DEBUG_LEVEL             2       // Print only debugs of this level or higher

    // Movement updates control
    #define MOVE_UPDATE_FREQUENCY   60      // Movement updates per second (~0.18KB/second/client)

    // Chat
    #define SERVER_MESSAGES         1       // Joined / Left / Game start / End messages
    #define PRINT_CHAT_TO_SERVER    0       // Turn the text display on the server on or off
    #define CHAT_DECRYPT_TIME       450     // seconds until chat is fully decrypted

    // Keep Alive Control
    #define CHECK_CONNECTIONS       0       // Toggle keep-alive protocols on or off
    #define CHECK_FREQUENCY         3       // Seconds between checks
    #define PRESUME_DEAD_FREQUENCY  5       // Seconds until loss is assumed

    // Floor control
    #define FLOOR_COP_START         3       // The floor the cops start on
    #define FLOOR_ROBBER_START      9       // The floor the robbers start on

    // Players
    #define MIN_PLAYERS             2       // min players to start the game
    #define BALANCE_TEAMS           1       // Toggle forced balancing on or off
    #define FAVOR_COPS              1       // Set cops to be favored over robbers in balancing

    // Objectives
    #define WIN_RATIO               0.75    // Percent of objective caputred that will end game

    // Start
    #define COUNTDOWN_TIME          5


// ************************************************************************************************
// ************************************************************************************************



// Includes
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
#include <math.h>

#include "NetComm.h"

// bool defs
#define TRUE    1
#define FALSE   0

// socket def
typedef int     SOCKET;


// ********************************************************************************


// server-util functions
packet_t    getPacketType(SOCKET socket);
int         getPacket(SOCKET socket, void* buffer, int sizeOfBuffer);

// game-util functions
void getSpawn(playerNo_t player, floorNo_t floor, pos_t* xPos, pos_t* yPos);

// min-util functions
void encapsulate_all_pos_update(PKT_ALL_POS_UPDATE *old_pkt, PKT_MIN_ALL_POS_UPDATE *pkt);
void decapsulate_pos_update(PKT_MIN_POS_UPDATE *pkt, PKT_POS_UPDATE* old_pkt);

// Keep alive functions
time_t clientHeartbeat[MAX_PLAYERS];
time_t serverHeartbeat[MAX_PLAYERS];
void clientPulse(playerNo_t plyr);
void serverPulse(playerNo_t plyr);

// Chat functions
void chatGameStart();
void sendChat(PKT_CHAT* chat, teamNo_t teams[MAX_PLAYERS], SOCKET outswitch);

typedef int     SOCKET;


// ********************************************************************************


// Controllers
void* ConnectionManager(void* ipcSocks);
void* InboundSwitchboard(void* ipcSocks);
void* GameplayController(void* ipcSocks);
void* GeneralController(void* ipcSocks);
void* UIController(void* ipcSocks);
void* OutboundSwitchboard(void* ipcSocks);
void* KeepAlive(void* outSock);
void* MovementTimer(void* ipcSocks);



// ********************************************************************************


// structures
#define NUM_IPC_PACKETS 5

typedef struct pktB0{
	char				serverName[MAX_NAME];
	int					maxPlayers;
} PKT_SERVER_SETUP;

#define IPC_PKT_0 0xB0

typedef struct pktB1{
	playerNo_t			playerNo;
	character_t         character;
	char 				playerName[MAX_NAME];
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



// ********************************************************************************


// MACROS
#define OUTMASK int_fast32_t
#define OUT_SET(mask, pos) mask|=(1<<(pos))
#define OUT_SETALL(mask) mask=0xFFFFFFFF
#define OUT_ZERO(mask) mask=0x00000000
#define OUT_ISSET(mask, pos) ((mask&(1<<(pos)))==(1<<(pos)))

#define DEBUG_INFO  1
#define DEBUG_WARN  2
#define DEBUG_ALRM  3
#define DEBUG(lvl, msg) if(DEBUG_ON && lvl >= DEBUG_LEVEL){printf("Debug: %s\n", msg);fflush(stdout);}


// ********************************************************************************

// global data stores
SOCKET 				tcpConnections[MAX_PLAYERS];
SOCKET				udpConnection;
struct sockaddr_in 	udpAddresses[MAX_PLAYERS];
int                 connectedPlayers;

int netPacketSizes[NUM_NET_PACKETS + 1];
int ipcPacketSizes[NUM_IPC_PACKETS + 1];
int largestNetPacket, largestIpcPacket, largestPacket;


// ********************************************************************************

#endif
