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

#ifndef SOCKET_FUNC
#define SOCKET_FUNC

int getPacketType(SOCKET socket);
int getPacket(SOCKET socket, void* buffer, int sizeOfBuffer);

#endif

