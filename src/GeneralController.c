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

int MIN_PLAYERS = 2;   // min players to trigger GAME_STATE_ACTIVE

// Utils
size_t getTeamCount(const teamNo_t *playerTeams, size_t *team1, size_t *team2);
teamNo_t findTeam(const int maxPlayers, const teamNo_t playerTeams[MAX_PLAYERS]);
int areRobbersLeft(const int maxPlayers, const teamNo_t playerTeams[MAX_PLAYERS]);
int areObjectivesCaptured(const int maxPlayers, const int objCaptured[MAX_OBJECTIVES]);
void writePacket(SOCKET sock, void* packet, packet_t type);
void balanceTeams(const teamNo_t *teamIn, teamNo_t *teamOut);

// Sending
void sendPlayerUpdate(const SOCKET sock, const bool_t* validities, const teamNo_t* teams,
     const status_t* statuses, char names[MAX_PLAYERS][MAX_NAME]);
status_t getGameStatus(const bool_t *validPlayers, const teamNo_t *playerTeams);


inline void sendGameStatus(SOCKET sock, PKT_GAME_STATUS *pkt);

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

    bool_t      validPlayers[MAX_PLAYERS];             // 1 in playerNo position notes player is valid
	char        playerNames[MAX_PLAYERS][MAX_NAME];   // stores the players name in the player number's spot
	teamNo_t    playerTeams[MAX_PLAYERS];              // stores the players team in the player number's spot
	teamNo_t    desiredTeams[MAX_PLAYERS];              // stores the team chosen by players in lobby
	bool_t      objCaptured[MAX_OBJECTIVES];           //
	status_t    playerStatus[MAX_PLAYERS];
	bool_t      needCheckWin = FALSE;

    status_t status = GAME_STATE_WAITING;              //

	size_t numPlayers = 0;                          // actual players connected count                           // actual players connected count
	packet_t inPktType, outPktType;

	int i, j, maxPlayers = -1;                                 // max players specified for teh game session
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

    bzero(validPlayers, sizeof(status_t)*MAX_PLAYERS);
    bzero(playerNames, sizeof(char)*MAX_PLAYERS*MAX_NAME);
    bzero(playerTeams, sizeof(teamNo_t)*MAX_PLAYERS);
    bzero(objCaptured, sizeof(bool_t)*MAX_OBJECTIVES);
    bzero(playerStatus, sizeof(status_t)*MAX_PLAYERS);
    /***** END GAME INIT *******/

	// Wait for IPC_PKT_0
	if ((inPktType = getPacketType(generalSock)) != IPC_PKT_0){
		fprintf(stderr, "Expected packet type: %d\nReceived: %d\n", IPC_PKT_0, inPktType);
		return NULL;
	}
	getPacket(generalSock, pkt0, ipcPacketSizes[0]);
	maxPlayers = pkt0->maxPlayers;

	// Zero out player teams and player names
	for(i = 0; i < MAX_PLAYERS; ++i)
	{
        // Set array space that will not hold players to invalid

        // STATE FOR OPEN PLAYER SLOT
        val = (i < maxPlayers) ? PLAYER_STATE_AVAILABLE: PLAYER_STATE_INVALID;
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
            if(numPlayers == maxPlayers)
            {
                // TO-DO: Alert client the game is full?
                DEBUG("GC> Game is full... player not added");
                break;
            }

			getPacket(generalSock, pkt1, ipcPacketSizes[1]);

            // Check if player slot is available
            if(validPlayers[pkt1->playerNo] != PLAYER_STATE_AVAILABLE)
            {
                #if DEBUG_ON
                    char buff[32];
                    sprintf(buff, "GC> Player Number: %d is taken!", pkt1->playerNo);
                    DEBUG(buff);
                #endif
                //TO-DO: Alert client?
                //break;
            }

            // Overwrite server data
            numPlayers++;

            // Assign no team (LOBBY!)
            playerTeams[pkt1->playerNo] = TEAM_NONE;
            strcpy(playerNames[pkt1->playerNo], pkt1->client_player_name);
            playerStatus[pkt1->playerNo] = PLAYER_STATE_WAITING;
            validPlayers[pkt1->playerNo] = TRUE;



            // Overwrite potential garbage
            memcpy(pktPlayersUpdate->otherPlayers_name, playerNames, sizeof(char)*MAX_PLAYERS*MAX_NAME);
            memcpy(pktPlayersUpdate->otherPlayers_teams, playerTeams, sizeof(teamNo_t)*MAX_PLAYERS);
            memcpy(pktPlayersUpdate->player_valid, validPlayers, sizeof(status_t)*MAX_PLAYERS);
            memcpy(pktPlayersUpdate->readystatus, playerStatus, sizeof(status_t)*MAX_PLAYERS);

            // Send Players Update Packet 3


            writePacket(outswitchSock, pktPlayersUpdate, 3);

            // Overwrite potential garbge
            pktGameStatus->game_status = status;
            memcpy(pktGameStatus->objectives_captured, objCaptured, sizeof(bool_t)*MAX_OBJECTIVES);

            // Send the Game Status Packet 8
            writePacket(outswitchSock, pktGameStatus, 8);
        break;
		case IPC_PKT_2: // Player Lost -> Sends pkt 3 Players Update
			DEBUG("GC> Received IPC_PKT_2 - Player Lost");
			if (numPlayers < 1)
			{
                DEBUG("GC> numPlayers < 1 HOW COULD WE LOSE SOMEONE?!");
                break;
			}

			getPacket(generalSock, pkt2, ipcPacketSizes[2]);
			if(!validPlayers[pkt2->playerNo])
			{
                DEBUG("GC> Lost player is not valid");
                break;
			}

			numPlayers--;

            // Overwrite server data
            validPlayers[pkt2->playerNo] = FALSE;
            *playerNames[pkt2->playerNo] = '\0';
            playerTeams[pkt2->playerNo] = TEAM_NONE;
            playerStatus[pkt2->playerNo] = PLAYER_STATE_DROPPED;

            sendPlayerUpdate(outswitchSock, validPlayers, playerTeams, playerStatus, playerNames);
            //DEBUG statement is in the function

            if(status == GAME_STATE_ACTIVE)
            {
                needCheckWin = TRUE;
                DEBUG("GC> Triggered win check - player lost");
            }
        break;
        case 5: // Ready Status change
            DEBUG("GC> Received packet 5 - Ready Status");
            getPacket(generalSock, pktReadyStatus, netPacketSizes[5]);
            if(!validPlayers[pktReadyStatus->player_number])
            {
                DEBUG("GC> Packet 5 was for an invalid player");
                break;
            }

            if(status == GAME_STATE_ACTIVE){
                break;
            }

            // Overwrite server data
            playerStatus[pktReadyStatus->player_number] = pktReadyStatus->ready_status;
            strcpy(playerNames[pktReadyStatus->player_number], pktReadyStatus->player_name);

            // Store his desired team
            desiredTeams[pktReadyStatus->player_number] = pktReadyStatus->team_number;

            status = getGameStatus(validPlayers, playerTeams);
            if(status == GAME_STATE_ACTIVE)
            {
                balanceTeams(desiredTeams, playerTeams);
                for(i = 0; validPlayers[i] == PLAYER_STATE_INVALID; i++)
                {
                    playerStatus[i] = PLAYER_STATE_ACTIVE;
                    pkt3->playerNo = i;
                    pkt3->newFloor = 1;
                }
            }

            //Overwrite potential garbage
            memcpy(pktPlayersUpdate->player_valid, validPlayers, sizeof(status_t)*MAX_PLAYERS);
            memcpy(pktPlayersUpdate->otherPlayers_name,playerNames, sizeof(char)*MAX_PLAYERS*MAX_NAME);
            memcpy(pktPlayersUpdate->otherPlayers_teams, playerTeams, sizeof(teamNo_t)*MAX_PLAYERS);
            memcpy(pktPlayersUpdate->readystatus, playerStatus, sizeof(status_t)*MAX_PLAYERS);

            // Send Players Update Packet 3 to everyone
            writePacket(outswitchSock, pktPlayersUpdate, 3);
        break;
		case 8: // Game Status
            DEBUG("GC> Received packet 8 - Game Status");
			getPacket(generalSock, pktGameStatus, netPacketSizes[8]);
			if (status == GAME_STATE_ACTIVE) {
				//update objective listing

				int countCaptured = 0;
				// Copy the client's objective listing to the server.
				// May need some better handling; if 2 are received very close to each other
				memcpy(objCaptured, pktGameStatus->objectives_captured, sizeof(bool_t)*MAX_OBJECTIVES);

				//check win
				for (i = 0; i < MAX_OBJECTIVES; i++)
					if (objCaptured[i] == 1)
						countCaptured++;

				if (countCaptured >= WIN_RATIO)
					status = GAME_STATE_OVER;
			}

            memcpy(pktGameStatus->objectives_captured, objCaptured, sizeof(bool_t)*MAX_OBJECTIVES);
            pktGameStatus->game_status = status;
            writePacket(outswitchSock, pktGameStatus, 8);
        break;
        case 14:
            DEBUG("GC> Received packet 14 - Player Tagged");
            getPacket(generalSock, pktTagging, netPacketSizes[14]);

            if(!validPlayers[pktTagging->taggee_id] || !validPlayers[pktTagging->tagger_id])
            {
                DEBUG("GC> Tagger or tagee is not valid");
                break;
            }

            pkt3->playerNo = pktTagging->taggee_id;
            pkt3->newFloor = FLOOR_LOBBY;

            outPktType = 0xB3;
            write(generalSock, &outPktType, sizeof(packet_t));
            write(generalSock, pkt3, ipcPacketSizes[3]);
            DEBUG("GC> Sent IPC packet 3 - Force move");

            playerTeams[pktTagging->taggee_id] = TEAM_NONE;
            playerStatus[pktTagging->taggee_id] = PLAYER_STATE_OUT;

            // Overwrite potential garbage
            memcpy(pktPlayersUpdate->readystatus, playerStatus, sizeof(status_t)*MAX_PLAYERS);
            memcpy(pktPlayersUpdate->otherPlayers_teams, playerTeams, sizeof(teamNo_t)*MAX_PLAYERS);
            memcpy(pktPlayersUpdate->otherPlayers_name, playerNames, sizeof(char)*MAX_PLAYERS*MAX_NAME);
            memcpy(pktPlayersUpdate->player_valid, validPlayers, sizeof(status_t)*MAX_PLAYERS);

            writePacket(outswitchSock, pktPlayersUpdate, 3);
            DEBUG("GC> Sent packet 3 - Players update");

            needCheckWin = TRUE;
            DEBUG("GC> Triggered win check - player tagged");
        break;
		default:
			DEBUG("GC> Receiving packets it shouldn't");
        break;
		}

		if(needCheckWin)
		{
            DEBUG("GC> Checking win condition");
		// TEAM 1 is cops
            if(!areRobbersLeft(maxPlayers, playerTeams))
                status = GAME_TEAM1_WIN;

            if(areObjectivesCaptured(maxPlayers, objCaptured))
                status = GAME_TEAM2_WIN;

            status = GAME_STATE_ACTIVE;

            pktGameStatus->game_status = status;
            memcpy(pktGameStatus->objectives_captured, objCaptured, sizeof(bool_t)*MAX_OBJECTIVES);
            writePacket(outswitchSock, pktGameStatus, 8);

            for(i =0; i < MAX_PLAYERS; i++)
            {
                playerTeams[i] = TEAM_NONE;
                playerStatus[i] = PLAYER_STATE_WAITING;
            }

            memcpy(pktPlayersUpdate->otherPlayers_teams, playerTeams, sizeof(teamNo_t)*MAX_PLAYERS);
            memcpy(pktPlayersUpdate->player_valid, validPlayers, sizeof(status_t)*MAX_PLAYERS);
            memcpy(pktPlayersUpdate->otherPlayers_name, playerNames, sizeof(char)*MAX_PLAYERS*MAX_NAME);
            memcpy(pktPlayersUpdate->readystatus, playerStatus, sizeof(status_t)*MAX_PLAYERS);
            writePacket(outswitchSock, pktPlayersUpdate, 3);

            for(i = 0; i < MAX_PLAYERS; i++)
            {
                if(validPlayers[i] == PLAYER_STATE_INVALID)
                    break;

                pkt3->newFloor = FLOOR_LOBBY;
                pkt3->playerNo = i;
                outPktType = 0xB3;
                write(generalSock, &outPktType, sizeof(packet_t));
                write(generalSock, pkt3, ipcPacketSizes[3]);
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
int areObjectivesCaptured(const int maxPlayers, const int objCaptured[MAX_OBJECTIVES])
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

// params [out] team1 - number of players in team1
//        [out] team2 - number of players in team2
// returns total number of players in the game
size_t getTeamCount(const teamNo_t *playerTeams, size_t *team1, size_t *team2)
{
    size_t i, state;
    *team1 = *team2 = 0;
    for(i = 0; i < MAX_PLAYERS; i++)
    {
        state = *(playerTeams+i);
        switch(state)
        {
            case PLAYER_STATE_INVALID: return (*team1) + (*team2);
            case TEAM_COPS:     (*team1)++; break;
            case TEAM_ROBBERS:  (*team2)++; break;
            default: DEBUG("GC> Error getting player's team"); break;
        }

    }
    return (*team1) + (*team2);
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

void balanceTeams(const teamNo_t *teamIn, teamNo_t *teamOut)
{
    size_t team1Count = 0, team2Count = 0, i = 0;
    memcpy(teamOut, teamIn, sizeof(teamNo_t)*MAX_PLAYERS);
    getTeamCount(teamIn, &team1Count, &team2Count);
    while(team1Count < team2Count)
    {
        if(*(teamOut+i) == TEAM_ROBBERS){
            *(teamOut+i) = TEAM_COPS;
            team1Count++;
            team2Count--;
        }
        i++;
    }
}


// counts ready players
// if enough players ready; returns GAME_STATE_ACTIVE
//      // else GAME_STATE_WAITING
status_t getGameStatus(const bool_t *validPlayers, const teamNo_t *playerTeams)
{
    status_t s = GAME_STATE_WAITING;
    size_t i;
    int playerCount = 0;
    int readyCount = 0;

    for(i = 0; i < MAX_PLAYERS; i++)
    {
        if(*(validPlayers+i) != PLAYER_STATE_INVALID){
            playerCount++;
        }
        if(!(validPlayers+i) == PLAYER_STATE_READY){
            readyCount++;
        }
    }

    if(playerCount >= MIN_PLAYERS && readyCount == playerCount){
        s = GAME_STATE_ACTIVE;
    }

    return s;
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


void sendPlayerUpdate(const SOCKET sock, const bool_t* validities, const teamNo_t* teams,
     const status_t* statuses, char names[MAX_PLAYERS][MAX_NAME])
{
    const int pType = 3;
    PKT_PLAYERS_UPDATE *pkt = malloc(netPacketSizes[pType]);

    // Overwrite potential garbage
    memcpy(pkt->player_valid,       validities, sizeof(pkt->player_valid));
    memcpy(pkt->otherPlayers_name,  names,      sizeof(pkt->otherPlayers_name));
    memcpy(pkt->otherPlayers_teams, teams,      sizeof(pkt->otherPlayers_teams));
    memcpy(pkt->readystatus,        statuses,   sizeof(pkt->readystatus));

    // Send packet 3 - player update
    writePacket(sock, pkt, pType);
    DEBUG("GC> Sent packet 3 - Players Update");
}
