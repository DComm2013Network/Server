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
void relayPacket(void* packet, int type);
	
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
	
	relayPacket(&setupPkt, ipcPacketSizes[0]);
	DEBUG("IS> Setup Complete");
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
	// Socket handling all being done by Con Man. Just get and relay
	void* packet = malloc(ipcPacketSizes[1]);
	int type;
	type = getPacketType(Inswitch_connectionSocket);
	getPacket(Inswitch_connectionSocket, packet, ipcPacketSizes[1]);
	relayPacket(packet, type);
	DEBUG("IS> Added new player");
}

void writeType(SOCKET sock, void* packet, int type){
	write(sock, &type, 1);
	if(type >= 0xB0){
		write(sock, packet, ipcPacketSizes[type - 0xB0]);
	}
	else{
		write(sock, packet, netPacketSizes[type]);
	}
}

void relayPacket(void* packet, int type){
	
	switch(type){
		
		// --------------------------IPC--------------------------------
		case 0xB0:		// Setup Packet
			writeType(Inswitch_connectionSocket, 	packet, type);
			writeType(Inswitch_outswitchSocket,		packet, type);
			writeType(Inswitch_gameplaySocket,		packet, type);
			writeType(Inswitch_generalSocket,		packet, type);
			break;
			
		case 0xB1:		// New player added
			writeType(Inswitch_outswitchSocket,		packet, type);
			writeType(Inswitch_gameplaySocket,		packet, type);
			writeType(Inswitch_generalSocket,		packet, type);
			break;
			
		case 0xB2:		// Client Disconnect
			writeType(Inswitch_connectionSocket, 	packet, type);
			writeType(Inswitch_outswitchSocket,		packet, type);
			writeType(Inswitch_gameplaySocket,		packet, type);
			writeType(Inswitch_generalSocket,		packet, type);
			break;
		
		// --------------------------NET--------------------------------
		
		case 0x01:
			break;
			
		case 0x02:
			break;
		
		case 0x03:
			break;
		
		case 0x04:
			break;
		
		case 0x05:
			break;
		
		case 0x06:
			break;
		
		case 0x07:
			break;
		
		case 0x08:		// Game Status
			writeType(Inswitch_generalSocket,		packet, type);
			break;
		
		case 0x09:
			break;
		
		case 0x10:		// Movement update
			writeType(Inswitch_gameplaySocket,		packet, type);
			break;
		
		case 0x11:
			break;
		
		case 0x12:
			break;
		
		case 0x13:
			break;
		
		default:
			DEBUG("IS> In Switchboard getting packets it shouldn't be");
			break;
	}
	
}



void getUdpInput(){
	
	int type = 0;
	int received;
	void* packet = malloc(sizeof(largestNetPacket + 1));
	
	// Get the datagram, we don't care about the client info for now
	received = recvFrom(udpConnection, &packet, largestNetPacket, NULL, NULL); 
	
	// The first bit should be the type
	type = *((int*)packet);
	
	if(received == netPacketSizes[type] + 1){
		relayPacket(packet + 1, type);
	}
	else{
		DEBUG("IS> UDP received incorrectly labled packet");
	}
	
	
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
int getTcpInput(int pos){

	int ctrl = 0;
	void* packet = malloc(largestPacket);
	 
	// Get the packet type
	ctrl = getPacketType(tcpConnections[pos]);
	
	
	if(ctrl == -1){
		// packet type failed, therefor socket is closed
		return 0;
	}
	
	getPacket(tcpConnections[pos], packet, netPacketSizes[ctrl]);
	
	relayPacket(packet, ctrl);
	
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
	bzero(&udpAddresses[pos], sizeof(struct sockaddr_in));
	
	lost.playerNo = pos;
	
	relayPacket(&lost, ipcPacketSizes[2]);
	
	DEBUG("IS> Connection closed");
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
	
	// Assign the global Sockets to make life so much easier
	Inswitch_uiSocket = ((SOCKET*)ipcSocks)[2];
	Inswitch_connectionSocket = ((SOCKET*)ipcSocks)[4]; 
	Inswitch_generalSocket = ((SOCKET*)ipcSocks)[0]; 
	Inswitch_gameplaySocket = ((SOCKET*)ipcSocks)[1];
	Inswitch_outswitchSocket = ((SOCKET*)ipcSocks)[3];
	
	DEBUG("IS> Inbound Switchboard started");
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
		
		// Add the master UPD socket
		FD_SET(udpConnection, &fdset);
		highSocket = (udpConnection > highSocket) ? udpConnection : highSocket;
		
		
		// Find all active Sockets
		numLiveSockets = select(highSocket + 1, &fdset, NULL, NULL, NULL);
		
		if(numLiveSockets == -1){
			DEBUG("IS> Select failed");
			perror("Select Failed in Inbound Switchboard!");
			continue;
		}
		
		// Check TCP sockets
		for(i = 0; i < MAX_PLAYERS; ++i){
			if(tcpConnections[i]){
				if(FD_ISSET(tcpConnections[i], &fdset)){
					if(!getTcpInput(i)){
						cleanupSocket(i);
					}
				}
			}
		}
		
		// Check master UDP socket
		if(FD_ISSET(udpConnection, &fdset)){
			getUdpInput();
		}
		
		// Check for incomming connection
		if(FD_ISSET(Inswitch_connectionSocket, &fdset)){
			addNewPlayer();
		}
		
	}
	
	return NULL;
}
