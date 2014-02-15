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
	/*
	INBOUND SWITCHBOARD
		allocate socket lists
		while 1
			listen on all sockets
			if new connection
				add new connection to socket to list of inbound sockets
				pass socket+info to outbound switchboard

			if movement packet
				pass to gameplay controller

			if game status packet
				pass to game status controller

				.... etc
	 */
	
	return -99;
}
