/*-------------------------------------------------------------------------------------------------------------------*
-- SOURCE FILE: InboundSwitchboard.c 	
--		The Process that will handle all traffic from already established client connections
--
-- FUNCTIONS:
-- 		int InboundSwitchboard(SOCKET connectionSockSet, SOCKET generalSockSet, SOCKET gameplaySockSet, 
--					SOCKET outswitchSockSet)
--
--
-- DATE: 		February 14, 2014
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

// Globals
SOCKET* tcpConnections;
SOCKET* udpConnections;
extern int RUNNING;
	

/*--------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	Get Input
--
-- DATE: 		February 17, 2014
--
-- REVISIONS: 	none
--
-- DESIGNER: 	Andrew Burian
--
-- PROGRAMMER: 	Andrew Burian
--
-- INTERFACE: 	void addNewPlayer()
--
-- RETURNS: 	void
--
-- NOTES:
-- Adds a new player based on the data provided by the connection manager.
-- Opens both a TCP and UDP connection to the new client
----------------------------------------------------------------------------------------------------------------------*/
void addNewPlayer(){
	
}

/*--------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	Get Input
--
-- DATE: 		February 17, 2014
--
-- REVISIONS: 	none
--
-- DESIGNER: 	Andrew Burian
--
-- PROGRAMMER: 	Andrew Burian
--
-- INTERFACE: 	int getInput(SOCKET liveSocket)
--
-- RETURNS: 	int
--					success: 1
--					failure: 0 - Socket Closed
--
-- NOTES:
-- Reads a live socket.
----------------------------------------------------------------------------------------------------------------------*/
int getInput(SOCKET liveSocket){
	/* Check to see if socket has died
	* 		remove if it has
	* read 1 byte
	* if it's a new connection, add it to the appropriate spot on the players list
	* if it's a message, copy and pass it to the appropriate controller
	* if game end, set running to false
	*/
	int ctrl = 0;
	 
	// Get the packet type
	if(read(liveSocket, &ctrl, 1) == 0)
	{
		return 0;
	}
	 
	// Master Functionality switch
	switch(ctrl){
		case IPC_PKT_1:	// New player added
			// Note: no need to send data to outbound switch, ConMan will do it.
			break;
		
		case 0x1:		// Shouldn't get these
		case 0x2:
		default:
			fprintf(stderr, "In Switchboard getting packets it shouldn't be\n");
			break;
	}
	
	return 1;
}

/*--------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	Cleanup Socket
--
-- DATE: 		February 17, 2014
--
-- REVISIONS: 	none
--
-- DESIGNER: 	Andrew Burian
--
-- PROGRAMMER: 	Andrew Burian
--
-- INTERFACE: 	void cleanupSocket(int pos, SOCKET conMan, SOCKET outSwitch)
--
-- RETURNS: 	void
--
-- NOTES:
-- Removes both TCP and UDP sockets for a certain player position, then notifies the connection manager and
-- the outbound switchboard to do likewise.
----------------------------------------------------------------------------------------------------------------------*/
void cleanupSocket(int pos, SOCKET conMan, SOCKET outSwitch){
	struct pktB2 lost;
	
	close(tcpConnections[pos]);
	close(udpConnections[pos]);
	
	tcpConnections[pos] = 0;
	udpConnections[pos] = 0;
	
	lost.playerNo = pos;
	
	write(conMan, &lost, sizeof(lost));
	write(outSwitch, &lost, sizeof(lost));
}

/*--------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	Inbound Switchboard
--
-- DATE: 		February 4, 2014
--
-- REVISIONS: 	none
--
-- DESIGNER: 	Andrew Burian
--
-- PROGRAMMER: 	Andrew Burian
--
-- INTERFACE: 	int InboundSwitchboard(SOCKET connectionSockSet, SOCKET generalSockSet, SOCKET gameplaySockSet, 
--					SOCKET outswitchSockSet)
--
-- RETURNS: 	int
--					failure:	-99 - Not yet implemented
--								-1  - Select failed
--								-2  - Connection Manager stopped responding
--					success: 	 0
--
-- NOTES:
-- 
----------------------------------------------------------------------------------------------------------------------*/
int InboundSwitchboard(SOCKET connectionSock, SOCKET generalSock, SOCKET gameplaySock, SOCKET outswitchSock){

	// Variable Declarations
	fd_set fdset;
	int numLiveSockets;
	SOCKET highSocket;
	
	int i;
	
	// Allocate space for all the sockets
	tcpConnections = malloc(sizeof(SOCKET) * MAX_PLAYERS);
	udpConnections = malloc(sizeof(SOCKET) * MAX_PLAYERS);
	memset(tcpConnections, 0, sizeof(SOCKET) * MAX_PLAYERS);
	memset(udpConnections, 0, sizeof(SOCKET) * MAX_PLAYERS);
	
	
	// Switchboard Functionallity
	while(RUNNING){
		
		FD_ZERO(&fdset);
		
		// Add Connection Socket
		FD_SET(connectionSock, &fdset);
		highSocket = connectionSock;
		
		// Add TCP connections to select
		for(i = 0; i < MAX_PLAYERS; ++i){
			if(tcpConnections[i]){
				FD_SET(tcpConnections[i], &fdset);
				highSocket = (tcpConnections[i] > highSocket) ? tcpConnections[i] : highSocket;
			}
		}
		
		// Add UDP sockets to select
		for(i = 0; i < MAX_PLAYERS; ++i){
			if(udpConnections[i]){
				FD_SET(udpConnections[i], &fdset);
				highSocket = (udpConnections[i] > highSocket) ? udpConnections[i] : highSocket;
			}
		}
		
		// Find all active Sockets
		numLiveSockets = select(highSocket + 1, &fdset, NULL, NULL, NULL);
		
		if(numLiveSockets == -1){
			perror("Select Failed in Inbound Switchboard!");
			return -1;
		}
		
		// Check TCP sockets
		for(i = 0; i < MAX_PLAYERS; ++i){
			if(tcpConnections[i]){
				if(FD_ISSET(tcpConnections[i], &fdset)){
					if(!getInput(tcpConnections[i])){
						cleanupSocket(i, connectionSock, outswitchSock);
					}
				}
			}
		}
		
		// Check UDP sockets
		for(i = 0; i < MAX_PLAYERS; ++i){
			if(udpConnections[i]){
				if(FD_ISSET(udpConnections[i], &fdset)){
					if(!getInput(udpConnections[i])){
						cleanupSocket(i, connectionSock, outswitchSock);
					}
				}
			}
		}
		
		// Check for incomming connection
		if(FD_ISSET(connectionSock, &fdset)){
			if(!getInput(connectionSock)){
				//Something has gone terribly wrong
				fprintf(stderr, "Connection lost to connection manager");
				return -2;
			}
		}
		// This is done last because I'm not sure how FD_ISSET would respond to a query to a
		//		handle that is not within it's set.

	}
	
	// Cleanup
	free(tcpConnections);
	free(udpConnections);
	
	return -99;
}
