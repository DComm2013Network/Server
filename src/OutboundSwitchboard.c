/*-------------------------------------------------------------------------------------------------------------------*
 -- SOURCE FILE: .c
 --		The Process that will ...
 --
 -- FUNCTIONS:
 -- 		int OutboundSwitchboard(SOCKET outswitchSock)
 --
 --
 -- DATE:		February 20, 2014
 --
 -- REVISIONS: 	none
 --
 -- DESIGNER:	Andrew Burian
 --
 -- PROGRAMMER:	Chris Holisky
 --
 -- NOTES:
 --
 *-------------------------------------------------------------------------------------------------------------------*/

#include "Server.h"

extern int RUNNING;




void sendToPlayers(int protocol, OUTMASK to, void* data, packet_t type){

	int i;
	void* packet = malloc(sizeof(packet_t) + netPacketSizes[type] + sizeof(timestamp_t));


	if(protocol == SOCK_STREAM){
		for(i = 0; i < MAX_PLAYERS; ++i){
			if(OUT_ISSET(to, i) && tcpConnections[i] != 0){
				send(tcpConnections[i], &type, sizeof(packet_t), 0);
				send(tcpConnections[i], data, netPacketSizes[type], 0);
			}
		}
	}
	else if(protocol == SOCK_DGRAM){
		for(i = 0; i < MAX_PLAYERS; ++i){
			if(OUT_ISSET(to, i) && tcpConnections[i] != 0){ // check tcp anyways, because will be valid even for udp
				*((packet_t*)packet) = type;
				memcpy((packet + sizeof(packet_t)), data, netPacketSizes[type]);
				*((timestamp_t*)packet + sizeof(packet_t) + netPacketSizes[type]) = clock();
				sendto(udpConnection, packet, sizeof(packet_t) + netPacketSizes[type] + sizeof(timestamp_t), 0,
                        (struct sockaddr*)&(udpAddresses[i]), sizeof(udpAddresses[i]));
			}
		}
	}
	return;
}




void handleOut(SOCKET liveSock){

	OUTMASK mask;
	int type;
	void* packet = malloc(largestPacket);

	// Get the type
	type = getPacketType(liveSock);

	// Get the data
	if(type >= 0xB0){
 		read(liveSock, packet, ipcPacketSizes[type - 0xB0]);
 	}
 	else{
 		read(liveSock, packet, netPacketSizes[type]);
        // get the mask
        read(liveSock, &mask, sizeof(OUTMASK));
	}

	// switch statement to deal with packets
	switch (type) {

		case 0xB0:
		case 0xB1:
		case 0xB2:
			// no need to do anything anymore. All handles are global.
			break;

		// TCP cases
		case 0x01:
		case 0x02:
		case 0x03:
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
		case 0x09:
		case 0x0c:
		case 0x0d:
			sendToPlayers(SOCK_STREAM, mask, packet, type);
			break;

		// UDP cases
		case 0x08:
		case 0x0a:
		case 0x0b:
			sendToPlayers(SOCK_DGRAM, mask, packet, type);
			break;
	}
}

/*--------------------------------------------------------------------------------------------------------------------
 -- FUNCTION:	OutboundSwitchboard
 --
 -- DATE:		February 20, 2014
 --
 -- REVISIONS: 	none
 --
 -- DESIGNER:	Andrew Burian / Chris Holisky
 --
 -- PROGRAMMER:	Chris Holisky
 --
 -- INTERFACE: 	int OutboundSwitchboard(SOCKET outswitchSock)
 SOCKET outswitchSock : inbound socket
 --
 -- RETURNS: 	int
 --					failure:	-99 Not yet implemented
 --					success: 	0
 --
 -- NOTES:
 --
 ----------------------------------------------------------------------------------------------------------------------*/
void* OutboundSwitchboard(void* ipcSocks){

	SOCKET inSw = ((SOCKET*)ipcSocks)[0];
	SOCKET game = ((SOCKET*)ipcSocks)[1];
	SOCKET genr = ((SOCKET*)ipcSocks)[2];

	int type;
	void* setup = malloc(ipcPacketSizes[0]);

	fd_set fdset;
	int numLiveSockets;
	SOCKET highSocket = (inSw>game)?((inSw>genr)?inSw:genr):((game>genr)?game:genr);

	DEBUG("OS> Outbound Switchboard started");

	// wait for IPC packet 0 - This is the server startup packet
	type = getPacketType(inSw);
	if(type != 0xB0){
		DEBUG("OS> setup getting packets it shouldn't be");
	}
	getPacket(inSw, setup, ipcPacketSizes[0]);

	DEBUG("OS> Setup Complete");

	while (RUNNING) {

		FD_ZERO(&fdset);

		FD_SET(inSw, &fdset);
		FD_SET(game, &fdset);
		FD_SET(genr, &fdset);

		numLiveSockets = select(highSocket + 1, &fdset, NULL, NULL, NULL);

		if(numLiveSockets == -1){
			DEBUG("OS> Select failed");
			continue;
		}

		if(FD_ISSET(inSw, &fdset)){
			handleOut(inSw);
		}
		if(FD_ISSET(game, &fdset)){
			handleOut(game);
		}
		if(FD_ISSET(genr, &fdset)){
			handleOut(genr);
		}

	}

	DEBUG("OS> Finished");

	return NULL;
}
