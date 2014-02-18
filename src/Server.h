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

//function prototypes
int UI(SOCKET outSock);
int ConnectionManager(SOCKET connectionSock, SOCKET outswitchSock);
int GameplayController(SOCKET gameplaySock, SOCKET outswitchSock);
int OutboundSwitchboard(SOCKET outswitchSock);
int InboundSwitchboard(SOCKET connectionSock, SOCKET generalSock, SOCKET gameplaySock, SOCKET outswitchSock);
int GeneralController(SOCKET generalSock, SOCKET outswitchSock);

// structures
typedef struct pktB1{
	SOCKET		newClientSock;
	playerNo_t	playerNo
} PKT_NEW_CLIENT;

#define IPC_PKT_1 0xB1

typedef struct pktB2{
	playerNo_t	playerNo
} PKT_LOST_CLIENT;

#define IPC_PKT_2 0xB2
