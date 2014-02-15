/*-------------------------------------------------------------------------------------------------------------------*
-- SOURCE FILE: InboundSwitchboard.c 	
--		The Process that will handle all traffic from already established client connections
--
-- FUNCTIONS:
-- 		int InboundSwitchboard(SOCKET connectionSockSet, SOCKET generalSockSet, SOCKET gameplaySockSet, 
--					SOCKET outswitchSockSet)
--
--
-- DATE: 		February 14, 2014
--
-- REVISIONS: 	none
--
-- DESIGNER: 	Andrew Burian
--
-- PROGRAMMER: 	Andrew Burian
--
-- NOTES:
-- 
*-------------------------------------------------------------------------------------------------------------------*/


#inlcude "NetComm.h"
#include "Server.h"

/*--------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	Main
--
-- DATE: 		February 4, 2014
--
-- REVISIONS: 	none
--
-- DESIGNER: 	Andrew Burian
--
-- PROGRAMMER: 	Andrew Burian
--
-- INTERFACE: 	int InboundSwitchboard(SOCKET connectionSockSet, SOCKET generalSockSet, SOCKET gameplaySockSet, 
--					SOCKET outswitchSockSet)
--
-- RETURNS: 	int
--					failure:	-99 Not yet implemented
--					success: 	0
--
-- NOTES:
-- 
----------------------------------------------------------------------------------------------------------------------*/
int InboundSwitchboard(SOCKET connectionSockSet, SOCKET generalSockSet, SOCKET gameplaySockSet, SOCKET outswitchSockSet){

	// Variable Declarations
	SOCKET* tcpConnections;
	SOCKET* udpConnections;
	
	// Allocate space for all the sockets
	tcpConnections = malloc(sizeof(SOCKET) * MAX_PLAYERS);
	udpConnections = malloc(sizeof(SOCKET) * MAX_PLAYERS);
	memset(tcpConnections, 0, sizeof(SOCKET) * MAX_PLAYERS);
	memset(udpConnections, 0, sizeof(SOCKET) * MAX_PLAYERS);
	
	// Switchboard Functionallity
	while(0){
 
		// Select on all sockets, recieve 1 int
		//		tpc sockets
		//		udp sockets
		//		connection socket
		
		// From live socket, receive the specified struct
		
		// Based on the struct, dispactch to the appropriate process

	}
	
	// Cleanup
	
	return -99;
}
