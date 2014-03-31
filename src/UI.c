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

inline void createSetupPacket(const char* servName, const int maxPlayers, PKT_SERVER_SETUP* pkt);
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
	char servName[MAX_NAME] = {0};
	PKT_SERVER_SETUP pkt;
	SOCKET outSock;
	packet_t pType = IPC_PKT_0;

    bzero(&pkt, ipcPacketSizes[0]);

	do{
        printf("Enter a server name: ");
	}while(scanf("%s", servName) != 1);

    if(!RUN_AT_LIMIT){
        do {
            printf("Enter the maximum number of players: ");
        } while(scanf("%d", &maxPlayers) != 1);
    }
    else{
        maxPlayers = MAX_PLAYERS;
    }

    if(maxPlayers > MAX_PLAYERS)
    {
        printf("Entered illegal number of players: %d\n", maxPlayers);
        maxPlayers = MAX_PLAYERS;
        printf("It has been reset to the maximum: %d\n", maxPlayers);
    }

	// create setup packet
	createSetupPacket(servName, maxPlayers, &pkt);
	printSetupPacketInfo(&pkt);

	// get the socket
	outSock = ((SOCKET*)ipcSocks)[0];

	// send setup packet
	write(outSock, &pType, sizeof(packet_t));
	write(outSock, &pkt, sizeof(pkt));

    printf("Server Running!\n\n");

	char input[24];
    // while running
	while(RUNNING)
	{
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

inline void createSetupPacket(const char* servName, const int maxPlayers, PKT_SERVER_SETUP* pkt) {
	bzero(pkt->serverName, MAX_NAME);
	strcpy(pkt->serverName, servName);
	pkt->maxPlayers = maxPlayers;
}

inline void printSetupPacketInfo(const PKT_SERVER_SETUP *pkt)
{
    printf("\n---\n");
	printf("Server name:\t%s\nMax Players:\t%d\n", pkt->serverName, pkt->maxPlayers);
	printf("---\n\n");
}

inline void listAllCommands()
{
	printf("Possible commands:\n");
	printf("quit\nget-stats\nhelp\npkt <type>\n");
}

void injectPacket(packet_t type, SOCKET out){
    void* data;

    PKT_CHAT                spoof_pkt4;
    PKT_READY_STATUS        spoof_pkt5;
    PKT_SPECIAL_TILE        spoof_pkt6;
    PKT_GAME_STATUS         spoof_pkt8;
    PKT_POS_UPDATE          spoof_pkt10;
    PKT_FLOOR_MOVE_REQUEST  spoof_pkt12;
    PKT_TAGGING             spoof_pkt14;

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

    bzero(&spoof_pkt4, netPacketSizes[4]);
    bzero(&spoof_pkt5, netPacketSizes[5]);
    bzero(&spoof_pkt6, netPacketSizes[6]);
    bzero(&spoof_pkt8, netPacketSizes[8]);
    bzero(&spoof_pkt10, netPacketSizes[10]);
    bzero(&spoof_pkt12, netPacketSizes[12]);
    bzero(&spoof_pkt14, netPacketSizes[14]);

    switch(type){

        case 4:
            printf("Send message from player no: ");
            while(!scanf("%d", &player)){}
            printf("Message: ");
            getchar(); // clear the \n
            getline(&string, &strSize, stdin);
            if(strSize > MAX_MESSAGE){
                strSize = MAX_MESSAGE;
                string[strSize - 1] = 0;
            }
            spoof_pkt4.sendingPlayer = player;
            memcpy(&spoof_pkt4.message, string, strSize);
            data = &spoof_pkt4;
            break;

        case 5:
            printf("Send Ready Status from player no: ");
            while(!scanf("%d", &player)){}
            spoof_pkt5.playerNumber = player;
            printf("Add to team: ");
            while(!scanf("%d", &team)){}
            spoof_pkt5.team_number = team;
            printf("Status: ");
            while(!scanf("%d", &status)){}
            spoof_pkt5.ready_status = status;
            printf("Name: ");
            getchar(); // clear the \n
            getline(&string, &strSize, stdin);
            if(strSize > MAX_MESSAGE){
                strSize = MAX_MESSAGE;
                string[strSize - 1] = 0;
            }
            memcpy(&spoof_pkt5.playerName, string, strSize);
            data = &spoof_pkt5;
            break;

        case 6:
            printf("Place special tile: ");
            while(!scanf("%d", &tile)){}
            spoof_pkt6.tile = tile;
            printf("On floor: ");
            while(!scanf("%d", &floor)){}
            spoof_pkt6.floor = floor;
            printf("At X: ");
            while(!scanf("%d", &pos)){}
            spoof_pkt6.xPos = pos;
            printf("At Y: ");
            while(!scanf("%d", &pos)){}
            spoof_pkt6.yPos = pos;
            data = &spoof_pkt6;
            break;

        case 8:
            printf("Capture objective: ");
            while(!scanf("%d", &player)){}
            spoof_pkt8.objectiveStates[player] = OBJECTIVE_CAPTURED;
            data = &spoof_pkt8;
            break;

        case 10:
            printf("Move player no: ");
            while(!scanf("%d", &player)){}
            spoof_pkt10.playerNumber = player;
            printf("On floor: ");
            while(!scanf("%d", &floor)){}
            spoof_pkt10.floor = floor;
            printf("To X: ");
            while(!scanf("%d", &pos)){}
            spoof_pkt10.xPos = pos;
            printf("To Y: ");
            while(!scanf("%d", &pos)){}
            spoof_pkt10.yPos = pos;
            printf("With Vel X: ");
            while(!scanf("%f", &vel)){}
            spoof_pkt10.xVel = vel;
            printf("With Vel Y: ");
            while(!scanf("%f", &vel)){}
            spoof_pkt10.yVel = vel;
            data = &spoof_pkt10;
            break;

        case 12:
            printf("Teleport player no: ");
            while(!scanf("%d", &player)){}
            spoof_pkt12.playerNumber = player;
            printf("From floor: ");
            while(!scanf("%d", &floor)){}
            spoof_pkt12.current_floor = floor;
            printf("To floor: ");
            while(!scanf("%d", &floor)){}
            spoof_pkt12.desired_floor = floor;
            printf("At X: ");
            while(!scanf("%d", &pos)){}
            spoof_pkt12.desired_xPos = pos;
            printf("At Y: ");
            while(!scanf("%d", &pos)){}
            spoof_pkt12.desired_yPos = pos;
            data = &spoof_pkt12;
            break;

        case 14:
            printf("Tag player no: ");
            while(!scanf("%d", &player)){}
            spoof_pkt14.taggee_id = player;
            printf("By player no: ");
            while(!scanf("%d", &player)){}
            spoof_pkt14.tagger_id = player;
            data = &spoof_pkt14;
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
