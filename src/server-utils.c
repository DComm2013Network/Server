/*---------------------------------------------------------------------------------------------------*
 -- HEADER FILE: ServerUtils.c 		A collection of function wrappers for socket I/O
 --
 -- PROGRAM:		Game-Server
 --
 -- FUNCTIONS:
 --		int getPacketType(SOCKET socket)
 --		int getPacket(SOCKET socket, void* buffer, int sizeOfBuffer)
 --
 -- DATE: 		    January 27, 2014
 --
 -- REVISIONS: 	    none
 --
 -- DESIGNER: 	    Andrew Burian
 --
 -- PROGRAMMER: 	Andrew Burian
 --
 -- NOTES:
 --
 -----------------------------------------------------------------------------------------------------*/
#include "Server.h"

packet_t getPacketType(SOCKET socket) {
	int type;
	int toRead = sizeof(packet_t);
	int thisRead;
	int totalRead = 0;

	while (toRead > 0) {

		thisRead = read(socket, &type + totalRead, toRead - totalRead);

		if (thisRead <= 0) {
			return -1;
		}

		totalRead += thisRead;
		toRead -= thisRead;
	}

	return type;
}

int getPacket(SOCKET socket, void* buffer, int sizeOfBuffer) {
	int toRead = sizeOfBuffer;
	int thisRead;
	int totalRead = 0;

	while (toRead > 0) {

		thisRead = read(socket, buffer + totalRead, toRead - totalRead);

		if (thisRead <= 0) {
			continue;
		}

		totalRead += thisRead;
		toRead -= thisRead;
	}

	return totalRead;
}

void serverAnnounce(struct sockaddr_in* client, socklen_t* clientLen) {
    PKT_SERVER_DISCOVER reply;
    void* packet = malloc(netPacketSizes[0x09] + sizeof(packet_t));

    // populate server name
    memcpy(reply.serverName, serverName, MAX_NAME);

    reply.playersCurrent = serverCurrentPlayers;
    reply.playersMax = serverMaxPlayers;
    reply.gameStatus = serverGameStatus;

    *((int*)packet) = 0x09;
    memcpy(&((PKT_SERVER_DISCOVER*)packet)[1], &reply, netPacketSizes[0x09]);

    sendto(udpConnection, packet, netPacketSizes[0x09] + sizeof(packet_t), 0, (struct sockaddr*)client, *clientLen);
}
