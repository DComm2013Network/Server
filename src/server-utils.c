/** @ingroup Server */
/** @{ */

/**
 * A collection of wrappers for packet I/O and Keep Alive functions
 *
 *
 * @file server-utils.c
 */

/** @} */
#include "Server.h"

/**
 * Gets a packet type or returns -1 on failure
 *
 * Revisions:
 *      -# none
 *
 * @param[in]   socket     The Socket to receive from
 * @return packet_t the type of packet
 *
 * @designer Andrew Burian
 * @author Andrew Burian
 *
 * @date January 27, 2014
 */
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

/**
 * Reads a packet of size into the provided buffer
 *
 * Revisions:
 *      -# none
 *
 * @param[in]   socket      The Socket to receive from
 * @param[in]   buffer      The buffer to read into
 * @param[in]   sizeOfBuffer    The amount of data to read
 * @return int the number of bytes read
 *
 * @designer Andrew Burian
 * @author Andrew Burian
 *
 * @date January 27, 2014
 */
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

/**
 * Notes when the server has sent a client something
 *
 * Revisions:
 *      -# none
 *
 * @param[in]   plyr     The player number
 * @return void
 *
 * @designer Andrew Burian
 * @author Andrew Burian
 *
 * @date March 2, 2014
 */
void serverPulse(playerNo_t plyr){
    serverHeartbeat[plyr] = time(NULL);
}

/**
 * Notes when the server has received something from a client
 *
 * Revisions:
 *      -# none
 *
 * @param[in]   plyr     The player number
 * @return void
 *
 * @designer Andrew Burian
 * @author Andrew Burian
 *
 * @date March 2, 2014
 */
void clientPulse(playerNo_t plyr){
    clientHeartbeat[plyr] = time(NULL);
}
