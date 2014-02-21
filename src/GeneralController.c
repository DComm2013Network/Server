/*-------------------------------------------------------------------------------------------------------------------*
-- SOURCE FILE: .c 	
--		The Process that will ...
--
-- FUNCTIONS:
-- 		int ...
--
--
-- DATE: 		
--
-- REVISIONS: 	none
--
-- DESIGNER: 	
--
-- PROGRAMMER: 	
--
-- NOTES:
-- 
*-------------------------------------------------------------------------------------------------------------------*/


#include "NetComm.h"
#include "Server.h"

/*--------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	...
--
-- DATE: 		20 February 2014
--
-- REVISIONS: 	none
--
-- DESIGNER: 	
--
-- PROGRAMMER: 	
--
-- INTERFACE: 	int ...
--
-- RETURNS: 	int
--					failure:	-99 Not yet implemented
--					success: 	0
--
-- NOTES:
-- 
----------------------------------------------------------------------------------------------------------------------*/
int GeneralController(SOCKET generalSock, SOCKET outswitchSock){
/*
if udp socket
	receive the size of the largest UDP packet
	determine which packet type it is
	cast the received data to the appropriate structure

	if packet 10 (Movement)
		pass to Gameplay
	
if tcp socket
	if socket is closed or terminated
		remove the socket and it's UDP counterpart
		create IPC packet 2 (Player lost) and pass it to Connection Manager, Outbound Switchboard, General, Gameplay
		close descriptors
	
	else
	
	read in the packet type
	fill the appropriate structure

	if packet 8 (gameplay update)
		pass packet to General

 */
	return -99;
}
