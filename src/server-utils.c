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


