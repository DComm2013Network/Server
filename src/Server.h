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
#include <errno.h>
#include <strings.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>

#define DEBUG_ON 1
#define DEBUG(msg) if(DEBUG_ON){printf("Debug: %s\n", msg);}

//function prototypes
int UI(SOCKET outSock);
int ConnectionManager(SOCKET connectionSock, SOCKET outswitchSock);
int GameplayController(SOCKET gameplaySock, SOCKET outswitchSock);
int OutboundSwitchboard(SOCKET outswitchSock);
int InboundSwitchboard(SOCKET uiSock, SOCKET connectionSock, SOCKET generalSock, SOCKET gameplaySock, SOCKET outswitchSock);
int GeneralController(SOCKET generalSock, SOCKET outswitchSock);

// structures
typedef struct pktB0{
	char				serverName[MAX_NAME];
	int					maxPlayers;
	int					port;
} PKT_SERVER_SETUP;

#define IPC_PKT_0 0xB0

typedef struct pktB1{
	SOCKET				newClientSock;
	playerNo_t			playerNo;
	char 				client_player_name[MAX_NAME];
	struct sockaddr_in 	addrInfo;
} PKT_NEW_CLIENT;

#define IPC_PKT_1 0xB1

typedef struct pktB2{
	playerNo_t			playerNo;
} PKT_LOST_CLIENT;

#define IPC_PKT_2 0xB2
