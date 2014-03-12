/*-------------------------------------------------------------------------------------------------------------------*
 -- SOURCE FILE: GeneralController.c
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
 -- PROGRAMMER: 	German Villarreal
 --
 -- NOTES:
 --
 *-------------------------------------------------------------------------------------------------------------------*/

#include "Server.h"

extern int RUNNING;
double WIN_RATIO = MAX_OBJECTIVES * 0.75;
inline void sendGameStatus(SOCKET sock, PKT_GAME_STATUS *pkt);
teamNo_t findTeam(const int maxPlayers, const teamNo_t playerTeams[MAX_PLAYERS]);
void writePacket(SOCKET sock, void* packet, packet_t type);
int areRobbersLeft(const int maxPlayers, const teamNo_t playerTeams[MAX_PLAYERS]);
int areObjectivesCaptured(const int maxPlayers, const objCaptured[MAX_OBJECTIVES]);

/*--------------------------------------------------------------------------------------------------------------------
 -- FUNCTION:	GeneralController
 --
 -- DATE: 		20 February 2014
 --
 -- REVISIONS: 	....
                11 March 2014
                pkt3 is being sent after IPC_PKT_1 received
 --
 -- DESIGNER:
 --
 -- PROGRAMMER: 	German Villarreal
 --
 -- INTERFACE: 	int GeneralController(SOCKET generalSock, SOCKET outswitchSock)
 --
 -- RETURNS: 	int
 --					failure:	-99 Not yet implemented
 --					success: 	0
 --
 -- NOTES:

 -- COMMUNICATIONS:
 -- IPC_PKT_0 -> Initializes server
 -- IPC_PKT_1 -> PKT3 | PKT 8
 -- IPC_PKT_2 -> PKT3
 -- PKT5 -> PKT3
 -- PKT8 -> PKT8
 -- PKT14 -> PKT3 | IPC3 -> check win

 --------------------------------------------------------------------------------------------------------------------*/

void* GeneralController(void* ipcSocks) {

    int         validPlayers[MAX_PLAYERS];             // 1 in playerNo position notes player is valid
	char        playerNames[MAX_PLAYERS][MAX_NAME];   // stores the players name in the player number's spot
	teamNo_t    playerTeams[MAX_PLAYERS];              // stores the players team in the player number's spot
	int         objCaptured[MAX_OBJECTIVES];           //
	int         playerStatus[MAX_PLAYERS];
	int         needCheckWin = 0;

    status_t status = GAME_STATE_WAITING;              //
	size_t numPlayers = 0;                             // actual players connected count
	packet_t inPktType;
	int i, j, maxPlayers;                                 // max players specified for teh game session
    teamNo_t val;

	/* Will look into changing this... */
	PKT_SERVER_SETUP    *pkt0;
	PKT_NEW_CLIENT      *pkt1;
	PKT_LOST_CLIENT     *pkt2;
	PKT_FORCE_MOVE      *pkt3;
	PKT_PLAYERS_UPDATE  *pktPlayersUpdate;  // net pkt 3
	PKT_READY_STATUS    *pktReadyStatus;    // net pkt 5
	PKT_GAME_STATUS     *pktGameStatus;     // net pkt 8
	PKT_FLOOR_MOVE      *pktFloorMove;      // net pkt 13
	PKT_TAGGING         *pktTagging;        // net pkt 14

	pkt0              = malloc(ipcPacketSizes[0]);
	pkt1              = malloc(ipcPacketSizes[1]);
	pkt2              = malloc(ipcPacketSizes[2]);
	pkt3              = malloc(ipcPacketSizes[3]);
	pktPlayersUpdate  = malloc(netPacketSizes[3]);
	pktReadyStatus    = malloc(netPacketSizes[5]);
	pktGameStatus     = malloc(netPacketSizes[8]);
	pktFloorMove      = malloc(netPacketSizes[13]);
	pktTagging        = malloc(netPacketSizes[14]);

	SOCKET generalSock   = ((SOCKET*) ipcSocks)[0];
	SOCKET outswitchSock = ((SOCKET*) ipcSocks)[1];
    /***** END GAME INIT *******/


	// Wait for IPC_PKT_0
	if ((inPktType = getPacketType(generalSock)) != IPC_PKT_0) {
		fprintf(stderr, "Expected packet type: %d\nReceived: %d\n", IPC_PKT_0, inPktType);
		return NULL;
	}
	getPacket(generalSock, pkt0, ipcPacketSizes[0]);
	maxPlayers = pkt0->maxPlayers;

	// Zero out player teams and player names
	for(i = 0; i < MAX_PLAYERS; ++i)
	{
        // Set the extra space that will not hold players to -1
        val = (i < maxPlayers) ? 0 : -1;
        playerTeams[i] = validPlayers[i] = playerStatus [i] = val;
        for(j = 0; j < MAX_NAME; j++)
            playerNames[i][j] = '\0';
	}
	DEBUG("GC> Setup Complete");

	while (RUNNING) {
		inPktType = getPacketType(generalSock);
		switch (inPktType) {
		case IPC_PKT_1: // New Player
            DEBUG("GC> Received IPC_PKT_1 - New Player");

			getPacket(generalSock, pkt1, ipcPacketSizes[1]);
			numPlayers++;

            // Overwrite server data
            playerTeams[pkt1->playerNo] = findTeam(maxPlayers, playerTeams);
            strcpy(playerNames[pkt1->playerNo], pkt1->client_player_name);
            playerStatus[pkt1->playerNo] = PLAYER_STATE_READY;
            validPlayers[pkt1->playerNo] = 1;

            // Overwrite potential garbage
            memcpy(pktPlayersUpdate->otherPlayers_name, playerNames, sizeof(playerNames));
            memcpy(pktPlayersUpdate->otherPlayers_teams, playerTeams, sizeof(playerTeams));
            memcpy(pktPlayersUpdate->player_valid, validPlayers, sizeof(validPlayers));
            memcpy(pktPlayersUpdate->readystatus, playerStatus, sizeof(playerStatus));

            // Send Players Update Packet 3
            writePacket(outswitchSock, pktPlayersUpdate, 0x03);

            // TO-DO: Change game status?

            // Overwrite potential garbge
            pktGameStatus->game_status = status;
            memcpy(pktGameStatus->objectives_captured, objCaptured, sizeof(objCaptured));

            // Send the Game Status Packet 8
            writePacket(outswitchSock, pktGameStatus, 0x08);
        break;
		case IPC_PKT_2: // Player Lost -> Sends pkt 3 Players Update
			DEBUG("GC> Received IPC_PKT_2 - Player Lost");
			if (numPlayers < 1)
			{
                DEBUG("GC> numPlayers < 1 HOW COULD WE LOSE SOMEONE?!");
                break;
			}

			getPacket(generalSock, pkt2, ipcPacketSizes[2]);
			numPlayers--;

            // Overwrite server data
            validPlayers[pkt2->playerNo] = 0;
            *playerNames[pkt2->playerNo] = '\0';
            playerTeams[pkt2->playerNo] = 0;
            playerStatus[pkt2->playerNo] = PLAYER_STATE_DROPPED;

            // Overwrite potential garbage
            memcpy(pktPlayersUpdate->player_valid, validPlayers, sizeof(validPlayers));
            memcpy(pktPlayersUpdate->otherPlayers_name, playerNames, sizeof(playerNames));
            memcpy(pktPlayersUpdate->otherPlayers_teams, playerTeams, sizeof(playerTeams));
            memcpy(pktPlayersUpdate->readystatus, playerStatus, sizeof(playerStatus));

            // Send packet 3 - player update
            writePacket(outswitchSock, pktPlayersUpdate, 0x03);
        break;
        case 0x05: // Ready Status change
            DEBUG("GC> Received packet 5 - Ready Status");
            getPacket(generalSock, pktReadyStatus, netPacketSizes[5]);
            if(!validPlayers[pktReadyStatus->player_number])
            {
                DEBUG("GC> Packet 5 was for an invalid player.");
                break;
            }

            // Overwrite server data
            playerStatus[pktReadyStatus->player_number] = pktReadyStatus->ready_status;

            //Overwrite potential garbage
            memcpy(pktPlayersUpdate->player_valid, validPlayers, sizeof(validPlayers));
            memcpy(pktPlayersUpdate->otherPlayers_name,playerNames, sizeof(playerNames));
            memcpy(pktPlayersUpdate->otherPlayers_teams, playerTeams, sizeof(playerTeams));
            memcpy(pktPlayersUpdate->readystatus, playerStatus, sizeof(playerStatus));

            // Send Players Update Packet 3 to everyone
            writePacket(outswitchSock, pktPlayersUpdate, 0x03);

        break;
		case 0x08: // Game Status
            DEBUG("GC> Received packet 8 - Game Status");
			getPacket(generalSock, pktGameStatus, netPacketSizes[8]);
			if (status == GAME_STATE_ACTIVE) {
				//update objective listing
				int countCaptured = 0;
				// Copy the client's objective listing to the server.
				// May need some better handling; if 2 are received very close to each other
				memcpy(objCaptured, pktGameStatus->objectives_captured, MAX_OBJECTIVES);

				//check win
				for (i = 0; i < MAX_OBJECTIVES; i++)
					if (objCaptured[i] == 1)
						countCaptured++;

				if (countCaptured >= WIN_RATIO)
					status = GAME_STATE_OVER;
			}

            memcpy(pktGameStatus->objectives_captured, objCaptured, MAX_OBJECTIVES);
            pktGameStatus->game_status = status;
            writePacket(outswitchSock, pktPlayersUpdate, 0x08);
        break;
        case 14:
            DEBUG("GC> Received packet 14 - Player Tagged");
            getPacket(generalSock, pktTagging, netPacketSizes[14]);

            pkt3->playerNo = pktTagging->taggee_id;
            pkt3->newFloor = FLOOR_LOBBY;

            write(generalSock, pkt3, ipcPacketSizes[3]);
            DEBUG("GC> Sent IPC packet 3 - Force move");

            playerTeams[pktTagging->taggee_id] = TEAM_NONE;
            playerStatus[pktTagging->taggee_id] = PLAYER_STATE_OUT;

            // Overwrite potential garbage
            memcpy(pktPlayersUpdate->readystatus, playerStatus, sizeof(playerStatus));
            memcpy(pktPlayersUpdate->otherPlayers_teams, playerTeams, sizeof(playerTeams));
            memcpy(pktPlayersUpdate->otherPlayers_name, playerNames, sizeof(playerNames));
            memcpy(pktPlayersUpdate->player_valid, validPlayers, sizeof(validPlayers));

            writePacket(outswitchSock, pktPlayersUpdate, 3);
            DEBUG("GC> Sent packet 3 - Players update");


            //DEBUG("GC> Triggered game condition");
        break;
		default:
			DEBUG("GC> Receiving packets it shouldn't");
        break;
		}

		if(needCheckWin)
		{
		// TEAM 1 is cops
            if(!areRobbersLeft(maxPlayers, playerTeams))
                status = GAME_TEAM1_WIN;

            if(areObjectivesCaptured(maxPlayers, objCaptured))
                status = GAME_TEAM2_WIN;

            status = GAME_STATE_ACTIVE;

            pktGameStatus->game_status = status;
            memcpy(pktGameStatus->objectives_captured, objCaptured, sizeof(objCaptured));
            writePacket(outswitchSock, pktGameStatus, 8);


            memset(playerTeams, TEAM_NONE, sizeof(playerTeams));
            memcpy(pktPlayersUpdate->otherPlayers_teams, playerTeams, sizeof(playerTeams));
            memcpy(pktPlayersUpdate->player_valid, validPlayers, sizeof(validPlayers));
            memcpy(pktPlayersUpdate->otherPlayers_name, playerNames, sizeof(playerNames));
            memcpy(pktPlayersUpdate->readystatus, playerStatus, sizeof(playerStatus));
            writePacket(outswitchSock, pktPlayersUpdate, 3);

            for(i = 0; i < maxPlayers; i++)
            {
                pkt3->newFloor = FLOOR_LOBBY;
                pkt3->playerNo = i;
                write(outswitchSock, pkt3, ipcPacketSizes[3]);
            }
            DEBUG("GC> Sent pkt3 - moving all to lobby; game is over");
		}
    }

   	free(pkt0);
	free(pkt1);
	free(pkt2);
	free(pkt3);
    free(pktPlayersUpdate);
    free(pktReadyStatus);
	free(pktGameStatus);
	free(pktFloorMove);
	free(pktTagging);

	return NULL;
}

// Checks for a winner
//  - objectives captured
//  - enough players in team
// Returns
//  - 1 - objectives captured
//  - 2 - players captured
//  - 0 if no winner yet
//  - -1 error
int areObjectivesCaptured(const int maxPlayers, const objCaptured[MAX_OBJECTIVES])
{
    int i, objCount = 0;
    for(i = 0; i < MAX_OBJECTIVES; i++) {
        objCount++;
    }

    if(objCount > WIN_RATIO * maxPlayers)
        return 1;

    return 0;
}

// Returns number of robbers left
int areRobbersLeft(const int maxPlayers, const teamNo_t playerTeams[MAX_PLAYERS])
{
    int i, robberCount = 0;
    for(i = 0; i < maxPlayers; i++)
        if(playerTeams[i] == TEAM_ROBBERS)
            robberCount++;

    return robberCount;
}

void writePacket(SOCKET sock, void* packet, packet_t type){
	OUTMASK m;
	OUT_SETALL(m);

	write(sock, &type, sizeof(packet_t));
    write(sock, packet, netPacketSizes[type]);
    write(sock, &m, sizeof(OUTMASK));

    #if DEBUG_ON
        char buff[32];
        sprintf(buff, "GC> Sent packet: %d", type);
        DEBUG(buff);
    #endif

}

teamNo_t findTeam(const int maxPlayers, const teamNo_t playerTeams[MAX_PLAYERS])
{
    int i;
    int team1Count = 0, team2Count = 0;
    for (i = 0; i < maxPlayers; i++)
    {
        if(playerTeams[i] == 1)
            team1Count++;
        if(playerTeams[i] == 2)
            team2Count++;
    }
    return (team1Count < team2Count) ? 1 : 2;
}

inline void sendGameStatus(SOCKET sock, PKT_GAME_STATUS *pkt)
{
    OUTMASK m;
    OUT_SETALL(m);
    packet_t pType = 0x08;

    write(sock, &pType, sizeof(packet_t));
    write(sock, pkt, netPacketSizes[8]);
    write(sock, &m, sizeof(OUTMASK));
    DEBUG("GC> Sent network packet 8 - Game Status");
}
