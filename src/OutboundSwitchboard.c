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




void sendToPlayers(int protocol, OUTMASK to, void* data, int len){
	
	int i;
	
	if(protocol == SOCK_STREAM){
		for(i = 0; i < MAX_PLAYERS; ++i){
			if(OUT_ISSET(to, i)){
				send(tcpConnections[i], data, len);
			}
		}
	}
	else if(protocol == SOCK_DGRAM){
		for(i = 0; i < MAX_PLAYERS; ++i){
			if(OUT_ISSET(to, i)){
				sendTo(udpConnection, data, len, udpAddresses[i], sizeof(udpAddresses[i]));
			}
		}
	}
	return;
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
	/*
	 * OUTBOUND SWITCHBOARD
	 while 1
	 listen on socket
	 if new connection
	 add tcp socket descriptor
	 open udp socket
	 else
	 forward to desired players
	 */
	//socket arrays
	SOCKET* tcpConnections;
	SOCKET* udpConnections;

	// Allocate space for all the sockets
	tcpConnections = malloc(sizeof(SOCKET) * MAX_PLAYERS);
	udpConnections = malloc(sizeof(SOCKET) * MAX_PLAYERS);
	memset(tcpConnections, 0, sizeof(SOCKET) * MAX_PLAYERS);
	memset(udpConnections, 0, sizeof(SOCKET) * MAX_PLAYERS);

	socklen_t addr_len = sizeof(struct sockaddr_in);

	bool playerList[MAX_PLAYERS];
	int playerCount = 0;
	int pType = -1;
	int i = 0;

	// Allocate buffer space for the message structs and zero them out
	// Calculate struct sizes so that the data is already in memory and
	//	processing won't be required once the server loop starts

	// IPC 0 - Server setup packet
	PKT_SERVER_SETUP *bufSetup = malloc(sizeof(PKT_SERVER_SETUP));
	memset(&bufSetup, 0, sizeof(PKT_SERVER_SETUP));
	int lenSetup = sizeof(PKT_SERVER_SETUP);

	// IPC 1 - Add player packet
	PKT_NEW_CLIENT *bufNewClient = malloc(sizeof(PKT_NEW_CLIENT));
	memset(&bufNewClient, 0, sizeof(PKT_NEW_CLIENT));
	int lenNewClient = sizeof(PKT_NEW_CLIENT);

	// IPC 2 - Drop player packet
	PKT_LOST_CLIENT *bufDropPlayer = malloc(sizeof(PKT_LOST_CLIENT));
	memset(&bufDropPlayer, 0, sizeof(PKT_LOST_CLIENT));
	int lenDropClient = sizeof(PKT_LOST_CLIENT);

	// pkt5 - Ready status packet
	PKT_READY_STATUS *bufReadyStatus = malloc(sizeof(PKT_READY_STATUS));
	memset(&bufReadyStatus, 0, sizeof(PKT_READY_STATUS));
	int lenReady = sizeof(PKT_READY_STATUS);

	// pkt8 - Objective status packet
	PKT_GAME_STATUS *bufObjectiveStatus = malloc(sizeof(PKT_GAME_STATUS));
	memset(&bufObjectiveStatus, 0, sizeof(PKT_GAME_STATUS));
	int lenObjective = sizeof(PKT_GAME_STATUS);

	// pkt11 - Floor update packet
	PKT_ALL_POS_UPDATE *bufPlayerFloorUpdate = malloc(
			sizeof(PKT_ALL_POS_UPDATE));
	memset(&bufPlayerFloorUpdate, 0, sizeof(PKT_ALL_POS_UPDATE));
	int lenFloorUpdate = sizeof(PKT_ALL_POS_UPDATE);

	// wait for IPC packet 0 - This is the server startup packet
	getPacket(outswitchSock, bufSetup, lenSetup);

	while (RUNNING) {
		// on each loop, reset packet type
		pType = -1;

		// block until data available from other processes
		pType = getPacketType(outswitchSock);

		// switch statement to deal with packets
		switch (pType) {

		//new player joined
		case 0xB1:
			if (getPacket(outswitchSock, bufNewClient, lenNewClient) == -1) {
				break;
			}
			/*
			 * add player's sockets to connection lists
			 *
			 * I have no idea what the hell I'm doing here... I'm too damn tired to understand
			 * 	any code at all right now......
			 */

				return -1;

			break;

			//player dropped
		case 0xB2:
			if (getPacket(outswitchSock, bufDropPlayer, lenDropClient) == -1) {
				break;
			}
			udpConnections[bufDropPlayer->playerNo] = 0;
			tcpConnections[bufDropPlayer->playerNo] = 0;
			break;

			// player info to send to all players
			// name, team, etc.
		case 5:
			if (getPacket(outswitchSock, bufReadyStatus, lenReady) == -1) {
				break;
			}
			for (i = 0; i < MAX_PLAYERS; i++) {
				if (udpConnections[i] != 0) {
					bufReadyStatus->timestamp = clock();

					/*
					 * send this packet to client[i]
					 */
				}
			}
			break;

			// game objective update to send to all players
		case 8:
			if (getPacket(outswitchSock, bufObjectiveStatus, lenObjective)
					== -1) {
				break;
			}
			for (i = 0; i < MAX_PLAYERS; i++) {
				if (udpConnections[i] != 0) {
					if (sendto(udpConnections[i], bufObjectiveStatus,
							lenObjective, 0, what structure?, addr_len) == -1)
						perror("Outbound packet 8 failed to send");
				}
				/*
				 * send this packet to client[i]
				 */
			}

		break;

		// position update for players on a given floor
		case 11:
		if (getPacket(outswitchSock, bufPlayerFloorUpdate, lenFloorUpdate)
				== -1) {
			break;
		}
		for (i = 0; i < MAX_PLAYERS; i++) {
			if (bufPlayerFloorUpdate->players_on_floor[i] == 1
					&& udpConnections[i] != 0) {
				bufPlayerFloorUpdate->timestamp = clock();

				/*
				 * send bufPlayerFloorUpdate to client[i]
				 */
			}
			break;
			default:
			break;
		}
	}
}

free( bufDropPlayer);
free( bufNewClient);
free( bufObjectiveStatus);
free( bufPlayerFloorUpdate);
free( bufReadyStatus);
free( bufSetup);
free( tcpConnections);
free( udpConnections);
return 0;
}
