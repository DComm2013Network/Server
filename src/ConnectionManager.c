/*-------------------------------------------------------------------------------------------------------------------*
-- SOURCE FILE: ConnectionManager.c 	
--		The Process that will deal with new connections being added, and potentially reconnecting lost ones.
--
-- FUNCTIONS:
-- 		int ConnectionManager(SOCKET connectionSock, SOCKET outswitchSock)
--
--
-- DATE: 		February 16, 2014
--
-- REVISIONS: 	none
--
-- DESIGNER: 	Andrew Burian
--
-- PROGRAMMER: 	Andrew Burian
--
-- NOTES:
-- 
*-------------------------------------------------------------------------------------------------------------------*/


#include "NetComm.h"
#include "Server.h"

// Everything Aman included in his example.
// Because to heck with it...
#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <strings.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>

// Globals
SOCKET listenSock;
SOCKET acceptSock;
int connectedPlayers[MAX_PLAYERS];


/*--------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	Add New Connection
--
-- DATE: 		February 16, 2014
--
-- REVISIONS: 	none
--
-- DESIGNER: 	Andrew Burian
--
-- PROGRAMMER: 	Andrew Burian
--
-- INTERFACE: 	void addNewConnection(int maxPlayers, SOCKET connectionSock, SOCKET outswitchSock)
--
-- RETURNS: 	void
--
-- NOTES:
-- Tries to add a new player to the server. If successful, notifies Switchboards.
-- If the server is full, client is rejected, and the connection is closed.
----------------------------------------------------------------------------------------------------------------------*/
void addNewConnection(int maxPlayers, SOCKET connectionSock, SOCKET outswitchSock){
	
	struct pktB1 newClientInfo;
	struct pkt01 clientReg;
	struct pkt02 replyToClient;
	struct sockaddr_in client;
	int addr_len = sizeof(struct sockaddr_in);
	int i;
	
	memset(&clientReg, 0, sizeof(struct pkt01));
	memset(&replyToClient, 0, sizeof(struck pkt02));
	
	if ((acceptSock = accept (listenSock, (struct sockaddr *)&client, &addr_len)) == -1){
		fprintf(stderr, "Cound not accept client\n");
		return;
	}
	
	// Read in the packet type, which should be 1
	if(!recv(acceptSock, &cmd, 1, 0)){
		perror("Failed to read in packet type for new connection");
		return;
	}
	
	// Register the new client if there is space available
	if(cmd == 1){
		if(recv(acceptSock, &clientReg, sizeof(struct pkt01)) != sizeof(struct pkt01)){
			perror("Failed to get client name");
		}
		
		for(i = 0; i < maxPlayers; ++i){
			if(connectedPlayers[i] == 0){
				break;
			}
		}
		
		// Add player if space
		if(i < maxPlayers){
			connectedPlayers[i] = acceptSock;
			replyToClient.connect_code = CONNECT_CODE_ACCEPTED;
			replyToClient.clients_player_number = i;
			
			// This is just for milestone 1, where first player is team 1, second is team 2
			replyToClient.clients_team_number = i;
			
			// Send accept to client with their player and team number
			send(acceptSock, &replyToClient, sizeof(struct pkt02), 0);
			
			newClientInfo.newClientSock = acceptSock;
			newClientInfo.playerNo = i;
			memcpy(&newClientInfo.client_player_name, &clientReg.client_player_name, MAX_NAME);
			memcpy(&newClientInfo.addrInfo, &client, addr_len);
			
			// Notify switchboards
			send(connectionSock, &newClientInfo, sizeof(struct pktB1), 0);
			send(outoutswitchSock, &newClientInfo, sizeof(struct pktB1), 0);
		}
		else{
			// Server is full. GTFO
			replyToClient.connect_code = CONNECT_CODE_DENIED;
			send(acceptSock, &replyToClient, sizeof(struct pkt02), 0);
			close(acceptSock);
		}
		
	}
	else{
		fprintf(stderr, "Connection Manager receiving packets it shouldn't be\n");
		return;
	}
	
	return;
}


/*--------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	Remove Connection
--
-- DATE: 		February 16, 2014
--
-- REVISIONS: 	none
--
-- DESIGNER: 	Andrew Burian
--
-- PROGRAMMER: 	Andrew Burian
--
-- INTERFACE: 	void removeConnection()
--
-- RETURNS: 	void
--
-- NOTES:
-- 
----------------------------------------------------------------------------------------------------------------------*/
void removeConnection();


/*--------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	Connection Manager
--
-- DATE: 		February 16, 2014
--
-- REVISIONS: 	none
--
-- DESIGNER: 	Andrew Burian
--
-- PROGRAMMER: 	Andrew Burian
--
-- INTERFACE: 	int ConnectionManager(SOCKET connectionSock, SOCKET outswitchSock)
--
-- RETURNS: 	int
--					failure:	-99 Not yet implemented
--								-1  Socket setup Failed
--					success: 	0
--
-- NOTES:
-- 
----------------------------------------------------------------------------------------------------------------------*/
int ConnectionManager(SOCKET connectionSock, SOCKET outswitchSock){
/*
 * CONNECTION MANAGER
    create, bind,
    loop
        listen, accept
        pass accepted socket+info to inbound switchboard

 */
	
	char cmd;
	int maxPlayers = MAX_PLAYERS;
	
	struct	sockaddr_in server;
	int addr_len = sizeof(struct sockaddr_in);
	
	
	listenSock = socket(AF_INET, SOCK_STREAM, 0);
	
	// No clients yet joined
	memset(clientsInUse, 0, MAX_PLAYERS * sizeof(int));
	
	memset(&server, 0, addr_len);
	
	// Set up the listening socket to any incoming address
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	
	// Bind
	if (bind(listenSock, (struct sockaddr *)&server, addr_len) == -1)
	{
		perror("Can't bind name to socket");
		return -1;
	}
	
	// queue up to 5 connect requests
	listen(listenSock, 5);
	
	while(RUNNING){
		
		
		
		
		
		
	}
	
	return -99;
}
