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

#define MAX_UDP_SIZE 1024

struct UDPReceive{
	int 	type
	byte	data[MAX_UDP_SIZE] // This should be set to the zise 
};


int getUDPPacket(SOCKET socket, struct UDPReceive* buf, struct sockaddr* addr, int addr_len){
	
	int thisRead;
	
	// receive as much of the packet as we can from the buffer
	// this assumes that data is at least as large as the largest UDP packet;
	thisRead = recvfrom(sd, buf, sizeof(struct UDPReceive), 0, (struct sockaddr *)&addr, &addr_len);
	
	if(thisRead == MAX_UDP_SIZE){
		return -2;
	}
	
	return thisRead;
	
}


int getPacketType(SOCKET socket){
	int type;
	int toRead = sizeof(int);
	int thisRead;
	int totalRead = 0;
	
	while(toRead > 0){
		
		thisRead = read(socket, &type + totalRead, toRead - totalRead);
		
		if(thisRead <= 0){
			return -1;
		}
		
		totalRead += thisRead;
		toRead -= thisRead;
	}
	
	
	
	return type;
}


int getPacket(SOCKET socket, void* buffer, int sizeOfBuffer){
	int toRead = sizeOfBuffer;
	int thisRead;
	int totalRead = 0;
	
	while(toRead > 0){
		
		thisRead = read(socket, buffer + totalRead, toRead - totalRead);
		
		if(thisRead <= 0){
			return -1;
		}
		
		totalRead += thisRead;
		toRead -= thisRead;
	}	
	
	return totalRead;
}
