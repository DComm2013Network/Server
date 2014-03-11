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
double winRatio = MAX_OBJECTIVES * 0.75;
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
 --
 ----------------------------------------------------------------------------------------------------------------------*/

void* GeneralController(void* ipcSocks) {

    int         validPlayers[MAX_PLAYERS];             // 1 in playerNo position notes player is valid
	char        playerNames[MAX_PLAYERS][MAX_NAME];   // stores the players name in the player number's spot
	teamNo_t    playerTeams[MAX_PLAYERS];              // stores the players team in the player number's spot
	int         objCaptured[MAX_OBJECTIVES];           //
	int         playerStatus[MAX_PLAYERS];

    status_t status = GAME_STATE_WAITING;              //
	size_t numPlayers = 0;                             // actual players connected count
	packet_t inPktType, pType;                                // value of the packet expected to read
	int i, maxPlayers;                                 // max players specified for teh game session
    int team1Count, team2Count;                        // number of players in the team
    teamNo_t addToTeam = 0, val;                            // the team to add a new player to

    OUTMASK m;

	/* Will look into changing this... */
	PKT_SERVER_SETUP    *pkt0;
	PKT_NEW_CLIENT      *pkt1;
	PKT_LOST_CLIENT     *pkt2;
	PKT_GAME_STATUS     *pktGameStatus;     // net pkt 8
	PKT_PLAYERS_UPDATE  *pktPlayersUpdate;  // net pkt 3
	PKT_READY_STATUS    *pktReadyStatus;

	pkt0              = malloc(ipcPacketSizes[0]);
	pkt1              = malloc(ipcPacketSizes[1]);
	pkt2              = malloc(ipcPacketSizes[2]);
	pktPlayersUpdate  = malloc(netPacketSizes[3]);
	pktReadyStatus    = malloc(netPacketSizes[5]);
	pktGameStatus     = malloc(netPacketSizes[8]);

	SOCKET generalSock   = ((SOCKET*) ipcSocks)[0];
	SOCKET outswitchSock = ((SOCKET*) ipcSocks)[1];
    /***** END GAME INIT *******/


	// wait ipc 0
	if ((inPktType = getPacketType(generalSock)) != IPC_PKT_0) {
		fprintf(stderr, "Expected packet type: %d\nReceived: %d\n", IPC_PKT_0, inPktType);
		return NULL;
	}
	getPacket(generalSock, pkt0, ipcPacketSizes[0]);

	maxPlayers = pkt0->maxPlayers;

	// Set the extra space that will not hold players to -1
	for(i = 0; i < MAX_PLAYERS; ++i)
	{
        val = (i < maxPlayers) ? 0 : -1;
        playerTeams[i] = val;
        playerNames[i][0] = 0;
	}
	DEBUG("GC> Setup Complete");

	while (RUNNING) {
		inPktType = getPacketType(generalSock);
		switch (inPktType) {

		case IPC_PKT_1: // New Player
            DEBUG("GC> Received IPC_PKT_1 - New Player");

			getPacket(generalSock, pkt1, ipcPacketSizes[1]);
			numPlayers++;

            // Find the best team to add a player to
            team1Count = 0; team2Count = 0;
            for (i = 0; i < maxPlayers; i++)
            {
                if(playerTeams[i] == 1)
                    team1Count++;
                if(playerTeams[i] == 2)
                    team2Count++;
            }
            addToTeam = (team1Count < team2Count) ? 1 : 2;

            /* Overwrite server data */
            // Add the player to the team
            playerTeams[pkt1->playerNo] = addToTeam;

            // Store the new player's name
            strcpy(playerNames[pkt1->playerNo], pkt1->client_player_name);

            // Set player status
            playerStatus[pkt1->playerNo] = PLAYER_STATE_READY;

            // Set the players data to be valid
            validPlayers[pkt1->playerNo] = 1;

            // Overwrite potential garbage
            memcpy(pktPlayersUpdate->otherPlayers_name, playerNames, sizeof(playerNames));
            memcpy(pktPlayersUpdate->otherPlayers_teams, playerTeams, sizeof(playerTeams));
            memcpy(pktPlayersUpdate->player_valid, validPlayers, sizeof(validPlayers));
            memcpy(pktPlayersUpdate->readystatus, playerStatus, sizeof(playerStatus));

            // Send Players Update Packet 3
            OUT_SETALL(m);
            pType = 0x03;
            write(outswitchSock, &pType, sizeof(packet_t));
            write(outswitchSock, pktPlayersUpdate, netPacketSizes[3]);
            write(outswitchSock, &m, sizeof(OUTMASK));
            DEBUG("GC> Sent network packet 3 - Players Update");

            // TO-DO: Change game status?

            // Overwrite potential garbge
            pktGameStatus->game_status = status;
            memcpy(pktGameStatus->objectives_captured, objCaptured, sizeof(objCaptured));

            // Send the Game Status Packet 8
            pType = 0x08;
            write(outswitchSock, &pType, sizeof(packet_t));
            write(outswitchSock, pktGameStatus, netPacketSizes[8]);
            write(outswitchSock, &m, sizeof(OUTMASK));
            DEBUG("GC> Sent network packet 8 - Game Status");

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
            OUT_SETALL(m);
            pType = 0x03;
            write(outswitchSock, &pType, sizeof(packet_t));
            write(outswitchSock, pktPlayersUpdate, netPacketSizes[3]);
            write(outswitchSock, &m, sizeof(OUTMASK));
            DEBUG("GC> Sent packet 3 - Players Update");
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
            OUT_SETALL(m);
            packet_t pType = 0x03;
            write(outswitchSock, &pType, sizeof(packet_t));
            write(outswitchSock, pktPlayersUpdate, netPacketSizes[3]);
            write(outswitchSock, &m, sizeof(OUTMASK));
            DEBUG("GC> Sent packet 3 - Players Update");
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

				if (countCaptured >= winRatio)
					status = GAME_STATE_OVER;
			}

            memcpy(pktGameStatus->objectives_captured, objCaptured, MAX_OBJECTIVES);
            pktGameStatus->game_status = status;
            sendGameStatus(outswitchSock, pktGameStatus);
            DEBUG("GC> Sent packet 8 - Game Status");
        break;
		default:
			DEBUG("GC> Receiving packets it shouldn't");
			break;
		}
    }

   	free(pkt0);
	free(pkt1);
	free(pkt2);
	free(pktGameStatus);

	return NULL;
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
