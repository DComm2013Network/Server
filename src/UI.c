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

inline PKT_SERVER_SETUP createSetupPacket(const char* servName, const int maxPlayers);
inline void printSetupPacketInfo(const PKT_SERVER_SETUP *pkt);
inline void listAllCommands();

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
--					1 when the server stops running
--
-- NOTES:
-- 
----------------------------------------------------------------------------------------------------------------------*/
void* UIController(void* ipSocks) {
	
	// prompt setup info
	const char *format = "%s %d";
	int maxPlayers = 0;
	char servName[MAX_NAME];
	PKT_SERVER_SETUP pkt;
	SOCKET outSock;
	int pType=IPC_PKT_0;
	
	do {
		printf("Enter serverName and maxPlayers as %s\n", format);	
	} while(scanf(format, servName, &maxPlayers) != 2);
	

	// create setup packet
	pkt = createSetupPacket(servName, maxPlayers);
	printSetupPacketInfo(&pkt);
	
	// get the socket
	outSock = ((SOCKET*)ipSocks)[0];
	
	// send setup packet
	write(outSock, &pType, 1);
	write(outSock, &pkt, sizeof(pkt));

	/* populate list of commands	
	char commands[3][15];
	strcpy(commands[0], "quit");
	strcpy(commands[1], "get-stats");
	strcpy(commands[2], "help"); 
	*/
	
	char input[24];
    // while running
	while(RUNNING)
	{
		// get input
		printf("Enter a command: ");
		if(scanf("%s", input) != 1)
		{
			printf("Error. Try that again..");
			continue;
		}
		        
        // if quit
		if(strcmp(input, "quit") == 0)
		{
			// create quit packet
            // send to switchboard
            // exit
		}
		
		// if get-stats
		if(strcmp(input, "get-stats") == 0)
		{
			printSetupPacketInfo(&pkt);
			continue;
		}
		
		//if help
		if(strcmp(input, "help") == 0)
		{
			listAllCommands();
			continue;
		}

        // if other
            // other
	}
	return 0;
}

inline PKT_SERVER_SETUP createSetupPacket(const char* servName, const int maxPlayers) {
	PKT_SERVER_SETUP pkt;
	strcpy(pkt.serverName, servName);
	pkt.maxPlayers = maxPlayers;
//	pkt.port = port;
	return pkt;
}

inline void printSetupPacketInfo(const PKT_SERVER_SETUP *pkt)
{
	printf("Server name:\t%s\nMax Players:\t%d\nPort:\t\t\n", pkt->serverName, pkt->maxPlayers);
}

inline void listAllCommands()
{
	printf("Possible commands:\n");
	printf("quit\nget-stats\nhelp\n");
}
