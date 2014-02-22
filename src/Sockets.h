/*---------------------------------------------------------------------------------------------------* 
 -- HEADER FILE: Server.h 		A collection of function wrappers for socket I/O
 --
 -- PROGRAM:		Game-Server
 --
 -- FUNCTIONS:
 -- 		int getUDPPacket(SOCKET socket, struct UDPReceive* buf, struct sockaddr* addr, int addr_len)
 --		int getPacketType(SOCKET socket)
 --		int getPacket(SOCKET socket, void* buffer, int sizeOfBuffer)
 --
 -- DATE: 		January 27, 2014
 --
 -- REVISIONS: 	none
 --
 -- DESIGNER: 	Andrew Burian
 --
 -- PROGRAMMER: 	Andrew Burian
 --
 -- NOTES:
 --
 -----------------------------------------------------------------------------------------------------*/

#ifndef SOCKET
#define SOCKET int
#endif

int getPacketType(SOCKET socket) {
	int type;
	int toRead = sizeof(int);
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
			return -1;
		}

		totalRead += thisRead;
		toRead -= thisRead;
	}

	return totalRead;
}
