/*-------------------------------------------------------------------------------------------------------------------*
-- SOURCE FILE: .c 	
--		The Process that will ...
--
-- FUNCTIONS:
-- 		int UI(SOCKET outSock)
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

extern int RUNNING;

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
-- INTERFACE: 	int UI(SOCKET outSock)
--
-- RETURNS: 	int
--					failure:	-99 Not yet implemented
--					success: 	0
--
-- NOTES:
-- 
----------------------------------------------------------------------------------------------------------------------*/
int UI(SOCKET outSock) {
	
	PKT_SERVER_SETUP pkt;

	// prompt setup info
	const char* format = "%s %d %d";
	printf("Enter serverName maxPlayers and port as %s\n", format);
	if(scanf(format, pkt.serverName, &pkt.maxPlayers, &pkt.port) != 3) {
		printf("Invalid arguments");
		break;
		
		printf(format, pkt.serverName, pkt.maxPlayers, pkt.port);
		printf("\n");
	}

	// pass IPC packet 0 to Inbound Switchboard		
	send(outSock, pkt, sizeof(pkt), 0);

	char input[
    // while running
	while(RUNNING)
	{
		// get input
		scanf("%s", input);
		
        // if quit
            // create quit packet
            // send to switchboard
            // exit
		if(strcmp(input, UI_QUIT) == 0)
		{
			;//quitaction
		}
		
		if(strcmp(
	}
    

        

        // if other
            // other
//	}
	return -99;
}
