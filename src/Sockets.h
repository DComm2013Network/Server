/* Collection of wrappers for sockets */

#ifndef SOCKET
#define SOCKET int
#endif

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
