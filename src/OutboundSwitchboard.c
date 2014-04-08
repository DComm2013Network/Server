/** @ingroup Server */
/** @{ */

/**
 * The Process will take packets from other threads and dispatch them to the appropriate players
 *
 * @file OutboundSwitchboard.c
 */

/** @} */
#include "Server.h"

extern int RUNNING;
void lostConnection(int pos);
sequence_t seq = 0;
SOCKET inSw;

/**
 * Sends packets to the appropriate players based on an outbound mask
 *
 * Revisions:
 *      -# None
 *
 * @param[in]  protocol				TCP / UDP
 * @param[in]  to          			The bitmask of players the packet should be sent to
 * @param[in]  data            		The packet data
 * @param[in]  type            		The type of packet being sent
 * @return void
 *
 * @designer Andrew Burian / Chris Holisky
 * @author Chris Holisky
 *
 * @date February 16, 2014
 */
void sendToPlayers(int protocol, OUTMASK to, void* data, packet_t type){

	int i, ret;
	void* packet = malloc(sizeof(packet_t) + netPacketSizes[type] + sizeof(sequence_t));


	if(protocol == SOCK_STREAM){
		for(i = 0; i < MAX_PLAYERS; ++i){
			if(OUT_ISSET(to, i) && tcpConnections[i] != 0){
			    serverPulse(i);
				if((ret = send(tcpConnections[i], &type, sizeof(packet_t), 0)) == -1){
                    lostConnection(i);
                }
				if((ret = send(tcpConnections[i], data, netPacketSizes[type], 0)) == -1){
                    lostConnection(i);
				}
			}
		}
	}
	else if(protocol == SOCK_DGRAM){
		for(i = 0; i < MAX_PLAYERS; ++i){
			if(OUT_ISSET(to, i) && tcpConnections[i] != 0){ // check tcp anyways, because will be valid even for udp
			    serverPulse(i);
				*((packet_t*)packet) = type;
				memcpy((packet + sizeof(packet_t)), data, netPacketSizes[type]);
				*((sequence_t*)(packet + sizeof(packet_t) + netPacketSizes[type])) = ++seq;
				sendto(udpConnection, packet, sizeof(packet_t) + netPacketSizes[type] + sizeof(sequence_t), 0,
                        (struct sockaddr*)&(udpAddresses[i]), sizeof(udpAddresses[i]));
			}
		}
	}

	free(packet);
	return;
}



/**
 * Outbound packet handler
 *
 * Revisions:
 *      -# None
 *
 * @param[in]   livesock      The live outbound socket
 * @return void
 *
 * @designer Andrew Burian / Chris Holisky
 * @author Chris Holisky
 *
 * @date February 16, 2014
 */
void handleOut(SOCKET liveSock){

	OUTMASK mask;
	int type;
	void* packet = malloc(largestPacket);

	// Get the type
	type = getPacketType(liveSock);

	if(type >= 0xB0){
	    // get data
 		read(liveSock, packet, ipcPacketSizes[type - 0xB0]);

 		// no mask
 	}
 	else if (type == KEEP_ALIVE){
        // don't read any packet data
        // keep alive has no data

        // it does need a mask though
        read(liveSock, &mask, sizeof(OUTMASK));
    }
    else{
        // get the data
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
        case 0x08:
		case 0x09:
		case 0x0c:
		case 0x0d:
			sendToPlayers(SOCK_STREAM, mask, packet, type);
			break;

		// UDP cases
		case 0x0a:
		case 0x0b:
        case 15:
		case 16:
			sendToPlayers(SOCK_DGRAM, mask, packet, type);
			break;
	}
	free(packet);
}


/**
 * Receives IPC packets from other methods and sends them to the appropriate clients
 *
 * Revisions:
 *      -# None
 *
 * @param[in]   ipcSocks      		The struct containing information for the sockets
 * @return void
 *
 * @designer Andrew Burian / Chris Holisky
 * @author Chris Holisky
 *
 * @date February 16, 2014
 */
void* OutboundSwitchboard(void* ipcSocks){

	SOCKET game;
	SOCKET genr;
	SOCKET kpal;

	int type;
	int highSocket = 0;
	void* setup = malloc(ipcPacketSizes[0]);

	fd_set fdset;
	int numLiveSockets;

    inSw = ((SOCKET*)ipcSocks)[0];
    highSocket = (inSw > highSocket) ? inSw : highSocket;
	game = ((SOCKET*)ipcSocks)[1];
	highSocket = (game > highSocket) ? game : highSocket;
	genr = ((SOCKET*)ipcSocks)[2];
	highSocket = (genr > highSocket) ? genr : highSocket;
	kpal = ((SOCKET*)ipcSocks)[3];
	highSocket = (kpal > highSocket) ? kpal : highSocket;

	DEBUG(DEBUG_INFO, "OS> Outbound Switchboard started");

	// wait for IPC packet 0 - This is the server startup packet
	type = getPacketType(inSw);
	if(type != 0xB0){
		DEBUG(DEBUG_ALRM, "OS> setup getting packets it shouldn't be");
	}
	getPacket(inSw, setup, ipcPacketSizes[0]);

	DEBUG(DEBUG_INFO, "OS> Setup Complete");

	while (RUNNING) {

		FD_ZERO(&fdset);

		FD_SET(inSw, &fdset);
		FD_SET(game, &fdset);
		FD_SET(genr, &fdset);
		FD_SET(kpal, &fdset);

		numLiveSockets = select(highSocket + 1, &fdset, NULL, NULL, NULL);

		if(numLiveSockets == -1){
			DEBUG(DEBUG_ALRM, "OS> Select failed");
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
		if(FD_ISSET(kpal, &fdset)){
            handleOut(kpal);
		}

	}

	DEBUG(DEBUG_INFO, "OS> Finished");

	return NULL;
}

/**
 * Sends lost connection packet to inbound switchboard
 *
 * Revisions:
 *      -# None
 *
 * @param[in]   pos				    The player position in the socket array
 * @return void
 *
 * @designer Andrew Burian / Chris Holisky
 * @author Chris Holisky
 *
 * @date February 16, 2014
 */
void lostConnection(int pos){
    PKT_LOST_CLIENT lost;
    packet_t lostType = IPC_PKT_2;

    bzero(&lost, ipcPacketSizes[2]);

    lost.playerNo = pos;

    write(inSw, &lostType, sizeof(packet_t));
    write(inSw, &lost, ipcPacketSizes[2]);
}
