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
-- DATE: 		
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
int OutboundSwitchboard(SOCKET outswitchSock){
/*
 * OUTBOUND SWITCHBOARD
    allocate tcp and udp socket list
    while 1
        listen on socket
        if new connection
            add tcp socket descriptor
            open udp socket
        else
            forward to esired players
 */
	return -99;
}
