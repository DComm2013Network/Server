/** @ingroup Server */
/** @{ */

/**
 * This file contains all methods responsible for the server's user console interface. This controller
 * is in charge of initializing the server and displaying relevant statistics and updates of the server's
 * operating conditions. Input is also read from the console to allow for packet injection.
 *
 *
 * @file UIController.c
 */

/** @} */

#include "NetComm.h"
#include "Server.h"

extern int RUNNING;

inline void createSetupPacket(const char* servName, const int maxPlayers, PKT_SERVER_SETUP* pkt);
inline void printSetupPacketInfo(const PKT_SERVER_SETUP *pkt);
inline void listAllCommands();
void injectPacket(packet_t type, SOCKET out);
void say(SOCKET out);
void tag(SOCKET out, playerNo_t player);

/**
 * Ongoing function while the server is running. Prompts setup info and listens for new commands.
 *
 * Revisions:
 *      -# March 28 - Added code from client mockup.
 *
 * @param[in]   ipcSocks     Array of 2 sockets for reading and writing.
 * @return void
 *
 * @designer German Villarreal
 * @author German Villarreal
 *
 * @date February 20, 2014
 */
void* UIController(void* ipcSocks) {

	// prompt setup info
	int maxPlayers = 0;
	packet_t type;
	char servName[MAX_NAME] = {0};
	PKT_SERVER_SETUP pkt;
	SOCKET outSock;
	packet_t pType = IPC_PKT_0;
	playerNo_t player = 0;

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
		if(strcmp(input, "stats") == 0)
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

		if(strcmp(input, "move") == 0){
            injectPacket(12, outSock);
            continue;
		}

        if(strcmp(input, "tag") == 0){
            if(scanf("%d", &player) == 1){
                tag(outSock, player);
            }
            continue;
		}

		if(strcmp(input, "pkt") == 0){
            if(scanf("%d", &type) == 1){
                injectPacket(type, outSock);
            }
            continue;
		}

		if(strcmp(input, "say") == 0){
            say(outSock);
            continue;
		}

        printf("Not a valid command. Try 'help' for commands.\n");
	}
	return 0;
}

/**
 * Initializesthe setup packet variables.
 *
 *
 * @param[in]   servName        Name of the server
 * @param[in]   maxPlayers      Maximum number of players
 * @param[out]   pkt            New setup packet.
 * @return void
 *
 * @designer German Villarreal
 * @author German Villarreal
 *
 * @date February 20, 2014
 */
inline void createSetupPacket(const char* servName, const int maxPlayers, PKT_SERVER_SETUP* pkt) {
	bzero(pkt->serverName, MAX_NAME);
	strcpy(pkt->serverName, servName);
	pkt->maxPlayers = maxPlayers;
}

/**
 * Prints the setup packet info.
 *
 * @param[in]   pkt            New setup packet.
 * @return void
 *
 * @designer German Villarreal
 * @author German Villarreal
 *
 * @date February 20, 2014
 */
inline void printSetupPacketInfo(const PKT_SERVER_SETUP *pkt)
{
    printf("\n---\n");
	printf("Server name:\t%s\nMax Players:\t%d\n", pkt->serverName, pkt->maxPlayers);
	printf("---\n\n");
}

/**
 * Prints a list of all available commands.
 *
 * @return void
 *
 * @designer German Villarreal
 * @author German Villarreal
 *
 * @date February 20, 2014
 */
inline void listAllCommands()
{
	printf("Possible commands:\n");
	printf(" - quit\n - stats\n - help\n - pkt <type>\n - say <message>\n - move\n - tag <player>\n");
	printf(" - pkt <pktNum>\n ");
}

/**
 * Prints a list of all available commands.
 *
 * @param[in] out       Socket to the main switchboard.
 * @param[in] player    Tagged player number.
 *
 * @return void
 *
 * @designer Andrew Burian
 * @author Andrew Burian
 *
 * @date February 20, 2014
 */
void tag(SOCKET out, playerNo_t player){
    packet_t type = 14;
    PKT_TAGGING tag;
    tag.taggee_id = player;
    tag.tagger_id = player;
    write(out, &type, sizeof(packet_t));
    write(out, &tag, netPacketSizes[type]);
}

/**
 * Creates and sends a chat packet as broadcast.
 *
 * @param[in] out       Socket to the main switchboard.
 *
 * @return void
 *
 * @designer Andrew Burian
 * @author Andrew Burian
 *
 * @date February 20, 2014
 */
void say(SOCKET out){

    PKT_CHAT chatPkt;
    bzero(&chatPkt, netPacketSizes[4]);
    char* string = 0;
    size_t strSize = 0;
    packet_t type = 4;
    int i;

    getchar(); // clear the space or new line
    getline(&string, &strSize, stdin);
    if(strSize > MAX_MESSAGE){
        strSize = MAX_MESSAGE;
        string[strSize - 1] = 0;
    }
    for(i = 0; i < strSize; ++i){
        if(string[i] == '\n'){
            string[i] = 0;
            break;
        }
    }
    chatPkt.sendingPlayer = MAX_PLAYERS;
    memcpy(&chatPkt.message, string, strSize);

    write(out, &type, sizeof(packet_t));
    write(out, &chatPkt, netPacketSizes[type]);
}

/**
 * Reads the specified packet to send and prompts for the user to enter the info.
 * Then sends the packet to the inbound swichboard for it to be processed as a regular packet.
 *
 * @param[in] type      The type of packet to send.
 * @param[in] out       Socket to the main switchboard.
 *
 * @return void
 *
 * @designer Chris Holisky
 * @author Chris Holisky
 *
 * @date February 20, 2014
 */
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
