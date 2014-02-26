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

inline PKT_SERVER_SETUP createSetupPacket(const char* servName, const int maxPlayers, const int port);
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
int UI(SOCKET outSock) {
	
	// prompt setup info
	const char *format = "%s %d %d";
	int maxPlayers = 0, port = 0;
	char servName[MAX_NAME];
	do {
		printf("Enter serverName maxPlayers and port as %s\n", format);	
	} while(scanf(format, servName, &maxPlayers, &port) != 3);
	PKT_SERVER_SETUP pkt = createSetupPacket(servName, maxPlayers, port);
	printSetupPacketInfo(&pkt);
	
	// pass IPC packet 0 to Inbound Switchboard
	send(outSock, &pkt, sizeof(pkt), 0);

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
	return 1;
}

inline PKT_SERVER_SETUP createSetupPacket(const char* servName, const int maxPlayers, const int port) {
	PKT_SERVER_SETUP pkt;
	strcpy(pkt.serverName, servName);
	pkt.maxPlayers = maxPlayers;
	pkt.port = port;
	return pkt;
}

inline void printSetupPacketInfo(const PKT_SERVER_SETUP *pkt)
{
	printf("Server name:\t%s\nMax Players:\t%d\nPort:\t\t%d\n", pkt->serverName, pkt->maxPlayers, pkt->port);
}

inline void listAllCommands()
{
	printf("Possible commands:\n");
	printf("quit\nget-stats\nhelp\n");
}
