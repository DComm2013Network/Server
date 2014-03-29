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
void injectPacket(packet_t type, SOCKET out);

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
void* UIController(void* ipcSocks) {

	// prompt setup info
	int maxPlayers = 0;
	packet_t type;
	char servName[MAX_NAME];
	PKT_SERVER_SETUP pkt;
	SOCKET outSock;
	packet_t pType=IPC_PKT_0;

	do{
        printf("Enter a server name: ");
	}while(scanf("%s", servName) != 1);


	do {
		printf("Enter the maximum number of players: ");
	} while(scanf("%d", &maxPlayers) != 1);

    if(maxPlayers > MAX_PLAYERS)
    {
        printf("Entered illegal number of players: %d\n", maxPlayers);
        maxPlayers = MAX_PLAYERS;
        printf("It has been reset to the maximum: %d\n", maxPlayers);
    }

	// create setup packet
	pkt = createSetupPacket(servName, maxPlayers);
	printSetupPacketInfo(&pkt);

	// get the socket
	outSock = ((SOCKET*)ipcSocks)[0];

	// send setup packet
	write(outSock, &pType, sizeof(packet_t));
	write(outSock, &pkt, sizeof(pkt));

    printf("Server Running!\n\n");
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
	    printf("> ");
		// get input
		if(scanf("%s", input) != 1)
		{
			printf("Error. Try that again..");
			continue;
		}

        // if quit
		if(strcmp(input, "quit") == 0)
		{
			// hard-kill
			// should really soften this blow
			exit(1);
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

		if(strcmp(input, "pkt") == 0){
            if(scanf("%d", &type) == 1){
                injectPacket(type, outSock);
            }
            continue;
		}

        printf("Not a valid command. Try 'help' for commands.\n");
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
	printf("Server name:\t%s\nMax Players:\t%d\n\n", pkt->serverName, pkt->maxPlayers);
}

inline void listAllCommands()
{
	printf("Possible commands:\n");
	printf("quit\nget-stats\nhelp\npkt <type>\n");
}

void injectPacket(packet_t type, SOCKET out){
    void* data;

    PKT_CHAT            pkt4;
    PKT_READY_STATUS    pkt5;
    PKT_SPECIAL_TILE    pkt6;

    playerNo_t player;
    teamNo_t team;
    status_t status;
    pos_t pos;
    vel_t vel;
    floorNo_t floor;
    tile_t tile;
    char* string = 0;
    size_t strSize = 0;

    int relay = 1;

    switch(type){
        case 4:
            printf("Send message from player no: ");
            while(!scanf("%d", &player)){}
            printf("Message: ");
            getchar(); // clear the \n
            getline(&string, &strSize, stdin);
            strSize = (strSize > MAX_MESSAGE) ? MAX_MESSAGE : strSize;
            pkt4.sendingPlayer = player;
            memcpy(&pkt4.message, string, strSize);
            data = &pkt4;
            break;

        case 5:
            printf("Send Ready Status from player no: ");
            while(!scanf("%d", &player)){}
            pkt5.playerNumber = player;
            printf("Add to team: ");
            while(!scanf("%d", &team)){}
            pkt5.team_number = team;
            printf("Status: ");
            while(!scanf("%d", &status)){}
            pkt5.ready_status = status;
            data = &pkt5;
            break;

        case 6:
            printf("Place special tile on floor: ");
            while(!scanf("%d", &floor)){}
            pkt6.floor = floor;
            printf("At X: ");
            while(!scanf("%d", &pos)){}
            pkt6.xPos = pos;
            printf("At Y: ");
            while(!scanf("%d", &pos)){}
            pkt6.yPos = pos;
            data = &pkt6;
            break;

        case 8:

            break;

        case 10:

            break;

        case 12:

            break;

        case 14:

            break;

        default:
            printf("Packet %d is not a spoofable packet\n", type);
            relay = 0;
            break;
    }

    if(relay){
        write(out, &type, sizeof(packet_t));
        write(out, data, netPacketSizes[type]);
    }

}
