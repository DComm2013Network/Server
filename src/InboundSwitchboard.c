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

SOCKET Inswitch_uiSocket, Inswitch_connectionSocket, Inswitch_generalSocket, Inswitch_gameplaySocket, Inswitch_outswitchSocket, Inswitch_keepAliveSocket, Inswitch_timerSocket;

void relayPacket(void* packet, packet_t type);

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
	packet_t type = 0xB0;

	if(getPacketType(Inswitch_uiSocket) != type){
		DEBUG(DEBUG_WARN, "IS> Inswitch setup getting packets it shouldn't be.");
		return;
	}

	getPacket(Inswitch_uiSocket, &setupPkt, ipcPacketSizes[0]);
	DEBUG(DEBUG_INFO, "IS> Got setup packet");
	relayPacket(&setupPkt, type);
	DEBUG(DEBUG_INFO, "IS> Setup Complete");
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
	packet_t type;
	type = getPacketType(Inswitch_connectionSocket);
	getPacket(Inswitch_connectionSocket, packet, ipcPacketSizes[1]);
	relayPacket(packet, type);
	DEBUG(DEBUG_INFO, "IS> Added new player");
	free(packet);
}

void getIPC(SOCKET sock){
    packet_t ctrl = 0;
	void* packet = malloc(largestIpcPacket);

	// Get the packet type
	ctrl = getPacketType(sock);
	getPacket(sock, packet, ipcPacketSizes[ctrl-0xB0]);

	relayPacket(packet, ctrl);

	free(packet);
}

void writeType(SOCKET sock, void* packet, packet_t type){
	write(sock, &type, sizeof(packet_t));
	if(type >= 0xB0){
		write(sock, packet, ipcPacketSizes[type - 0xB0]);
	}
	else{
		write(sock, packet, netPacketSizes[type]);
	}
}

void relayPacket(void* packet, packet_t type){

	switch(type){

		// --------------------------IPC--------------------------------
		case 0xB0:		// Setup Packet
			writeType(Inswitch_connectionSocket, 	packet, type);
			writeType(Inswitch_outswitchSocket,		packet, type);
			writeType(Inswitch_gameplaySocket,		packet, type);
			writeType(Inswitch_generalSocket,		packet, type);
			writeType(Inswitch_timerSocket,         packet, type);
			DEBUG(DEBUG_INFO, "IS> Routed pkt B0");
			break;

		case 0xB1:		// New player added
			writeType(Inswitch_outswitchSocket,		packet, type);
			writeType(Inswitch_gameplaySocket,		packet, type);
			writeType(Inswitch_generalSocket,		packet, type);
			DEBUG(DEBUG_INFO, "IS> Routed pkt B1");
			break;

		case 0xB2:		// Client Disconnect
			writeType(Inswitch_connectionSocket, 	packet, type);
			writeType(Inswitch_outswitchSocket,		packet, type);
			writeType(Inswitch_gameplaySocket,		packet, type);
			writeType(Inswitch_generalSocket,		packet, type);
			DEBUG(DEBUG_INFO, "IS> Routed pkt B2");
			break;

        case 0xB3:      // Forced floor change
            writeType(Inswitch_gameplaySocket,      packet, type);
            DEBUG(DEBUG_INFO, "IS> Routed pkt B3");
            break;

        case 0xB4:
            writeType(Inswitch_gameplaySocket,      packet, type);
            DEBUG(DEBUG_INFO, "IS> Routed pkt B4");
            break;
 		// --------------------------NET--------------------------------

		case 1:
			break;

		case 2:
			break;

		case 3:
			break;

		case 4:
			break;

		case 5:
            writeType(Inswitch_generalSocket,       packet, type);
            DEBUG(DEBUG_INFO, "IS> Routed pkt 5");
			break;

		case 6:
			break;

		case 7:
			break;

		case 8:		// Game Status
			writeType(Inswitch_generalSocket,		packet, type);
			DEBUG(DEBUG_INFO, "IS> Routed pkt 8");
			break;

		case 9:
			break;

		case 10:		// Movement update
			writeType(Inswitch_gameplaySocket,		packet, type);
			//DEBUG("IS> Routed pkt 10");
			break;

		case 11:
			break;

		case 12:
            writeType(Inswitch_gameplaySocket,      packet, type);
            DEBUG(DEBUG_INFO, "IS> Routed pkt 12");
            break;

		case 13:
			break;

        case 14:
            writeType(Inswitch_generalSocket,       packet, type);
            DEBUG(DEBUG_INFO, "IS> Routed pkt 14");
            break;

		default:
			DEBUG(DEBUG_ALRM, "IS> In Switchboard getting packets it shouldn't be");
			break;
	}

}



void getUdpInput(){

	packet_t type = 0;
	int received;
	void* packet = malloc(largestNetPacket + sizeof(packet_t));

	// Get the datagram, we don't care about the client info for now
	received = recvfrom(udpConnection, packet, largestNetPacket, 0, NULL, NULL);

	// The first bit should be the type
	type = *((packet_t*)packet);

	if(received == netPacketSizes[type] + sizeof(packet_t)){
		relayPacket(packet + sizeof(packet_t), type);
	}
	else{
		DEBUG(DEBUG_ALRM, "IS> UDP received incorrectly labled packet");
	}

    //free(packet);

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

	packet_t ctrl = 0;
	void* packet = malloc(largestPacket);

	// Get the packet type
	ctrl = getPacketType(tcpConnections[pos]);


	if(ctrl == -1){
		// packet type failed, therefor socket is closed
		free(packet);
		return 0;
	}
	if(ctrl == KEEP_ALIVE){
        // ignore
        free(packet);
        return 1;
	}

	getPacket(tcpConnections[pos], packet, netPacketSizes[ctrl]);

	relayPacket(packet, ctrl);

	free(packet);

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

	relayPacket(&lost, IPC_PKT_2);

	DEBUG(DEBUG_INFO, "IS> Connection closed");
}

void removePlayer(SOCKET sock){
    struct pktB2 lost;

    if(getPacketType(sock) != IPC_PKT_2){
        DEBUG(DEBUG_ALRM, "IS> Remove Player getting packets from outSwitch it shouldn't.");
        return;
    }

    getPacket(sock, &lost, ipcPacketSizes[2]);

	close(tcpConnections[lost.playerNo]);

	tcpConnections[lost.playerNo] = 0;
	bzero(&udpAddresses[lost.playerNo], sizeof(struct sockaddr_in));

	lost.playerNo = lost.playerNo;

	relayPacket(&lost, IPC_PKT_2);

	DEBUG(DEBUG_WARN, "IS> Connection closed by outbound switch");
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
	Inswitch_keepAliveSocket = ((SOCKET*)ipcSocks)[5];
	Inswitch_timerSocket = ((SOCKET*)ipcSocks)[6];

	DEBUG(DEBUG_INFO, "IS> Inbound Switchboard started");
	inswitchSetup();

	// Switchboard Functionallity
	while(RUNNING){

		FD_ZERO(&fdset);

		// Add Connection Socket
		FD_SET(Inswitch_connectionSocket, &fdset);
		FD_SET(Inswitch_generalSocket, &fdset);
		highSocket = (Inswitch_connectionSocket > Inswitch_generalSocket) ? Inswitch_connectionSocket: Inswitch_generalSocket;

		// Add TCP connections to select
		for(i = 0; i < MAX_PLAYERS; ++i){
			if(tcpConnections[i]){
				FD_SET(tcpConnections[i], &fdset);
				highSocket = (tcpConnections[i] > highSocket) ? tcpConnections[i] : highSocket;
			}
		}

        // Add the timer socket
		FD_SET(Inswitch_timerSocket, &fdset);
		highSocket = (Inswitch_timerSocket > highSocket) ? Inswitch_timerSocket : highSocket;

		// Add the master UPD socket
		FD_SET(udpConnection, &fdset);
		highSocket = (udpConnection > highSocket) ? udpConnection : highSocket;

        FD_SET(Inswitch_keepAliveSocket, &fdset);
        highSocket = (Inswitch_keepAliveSocket > highSocket) ? Inswitch_keepAliveSocket : highSocket;

		// Find all active Sockets
		numLiveSockets = select(highSocket + 1, &fdset, NULL, NULL, NULL);

		if(numLiveSockets == -1){
			DEBUG(DEBUG_ALRM, "IS> Select failed");
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
					else{
                        clientPulse(i);
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


		if(FD_ISSET(Inswitch_outswitchSocket, &fdset)){
            removePlayer(Inswitch_outswitchSocket);
		}

		if(FD_ISSET(Inswitch_generalSocket, &fdset)){
            getIPC(Inswitch_generalSocket);
		}

        if(FD_ISSET(Inswitch_keepAliveSocket, &fdset)){
            removePlayer(Inswitch_keepAliveSocket);
        }
		if(FD_ISSET(Inswitch_timerSocket, &fdset)){
            getIPC(Inswitch_timerSocket);
		}

	}

	return NULL;
}
