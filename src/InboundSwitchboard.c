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

#include "Server.h"

// Globals
extern int RUNNING;

SOCKET Inswitch_uiSocket, Inswitch_connectionSocket, Inswitch_generalSocket, Inswitch_gameplaySocket, Inswitch_outswitchSocket;

void* inswitch_packet;
	
/*--------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	In-Switch Setup
--
-- DATE: 		February 19, 2014
--
-- REVISIONS: 	none
--
-- DESIGNER: 	Andrew Burian
--
-- PROGRAMMER: 	Andrew Burian
--
-- INTERFACE: 	void inswtichSetup()
--
-- RETURNS: 	void
--
-- NOTES:
-- Receives the setup packet from the UI and passes it to all who need it.
----------------------------------------------------------------------------------------------------------------------*/
void inswitchSetup(){
	struct pktB0 setupPkt;
	int type = 0xB0;
	
	if(getPacketType(Inswitch_uiSocket) != type){
		DEBUG("IS> Inswitch setup getting packets it shouldn't be.");
		return;
	}
	
	getPacket(Inswitch_uiSocket, &setupPkt, ipcPacketSizes[0]);
	
	write(Inswitch_connectionSocket, &type, 1);
	write(Inswitch_connectionSocket, &setupPkt, ipcPacketSizes[0]);
	
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
	ctrl = getPacketType(liveSocket);
	 
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
-- INTERFACE: 	void cleanupSocket(int pos)
--
-- RETURNS: 	void
--
-- NOTES:
-- Removes both TCP and UDP sockets for a certain player position, then notifies the connection manager and
-- the outbound switchboard to do likewise.
----------------------------------------------------------------------------------------------------------------------*/
void cleanupSocket(int pos){
	struct pktB2 lost;
	
	close(tcpConnections[pos]);
	
	tcpConnections[pos] = 0;
	bzero(&udpConnections[pos], sizeof(struct sockaddr_in));
	
	lost.playerNo = pos;
	
	write(Inswitch_connectionSocket, &lost, sizeof(lost));
	write(Inswitch_outswitchSocket, &lost, sizeof(lost));
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
void* InboundSwitchboard(void* ipcSocks){

	// Variable Declarations
	fd_set fdset;
	int numLiveSockets;
	SOCKET highSocket;
	
	int i;
	packet = malloc(largestPacket + 1);
	
	// Assign the global Sockets to make life so much easier
	Inswitch_uiSocket = ((SOCKET*)ipcSocks)[2];
	Inswitch_connectionSocket = ((SOCKET*)ipcSocks)[4]; 
	Inswitch_generalSocket = ((SOCKET*)ipcSocks)[0]; 
	Inswitch_gameplaySocket = ((SOCKET*)ipcSocks)[1];
	Inswitch_outswitchSocket = ((SOCKET*)ipcSocks)[3];
	
	inswitchSetup();
	
	// Switchboard Functionallity
	while(RUNNING){
		
		FD_ZERO(&fdset);
		
		// Add Connection Socket
		FD_SET(Inswitch_connectionSocket, &fdset);
		highSocket = Inswitch_connectionSocket;
		
		// Add TCP connections to select
		for(i = 0; i < MAX_PLAYERS; ++i){
			if(tcpConnections[i]){
				FD_SET(tcpConnections[i], &fdset);
				highSocket = (tcpConnections[i] > highSocket) ? tcpConnections[i] : highSocket;
			}
		}
		
		// <<<<TODO UDP SOCKET>>>>>>
		
		
		
		// Find all active Sockets
		numLiveSockets = select(highSocket + 1, &fdset, NULL, NULL, NULL);
		
		if(numLiveSockets == -1){
			perror("Select Failed in Inbound Switchboard!");
			return NULL;
		}
		
		// Check TCP sockets
		for(i = 0; i < MAX_PLAYERS; ++i){
			if(tcpConnections[i]){
				if(FD_ISSET(tcpConnections[i], &fdset)){
					if(!getInput(tcpConnections[i])){
						//cleanupSocket(i, connectionSock, outswitchSock);
					}
				}
			}
		}
		
		// <<<< TODO CHECK UDP >>>>>
		
		// Check for incomming connection
		if(FD_ISSET(Inswitch_connectionSocket, &fdset)){
			if(!getInput(Inswitch_connectionSocket)){
				//Something has gone terribly wrong
				fprintf(stderr, "Connection lost to connection manager");
				return NULL;
			}
		}
		// This is done last because I'm not sure how FD_ISSET would respond to a query to a
		//		handle that is not within it's set.

	}
	
	return NULL;
}
