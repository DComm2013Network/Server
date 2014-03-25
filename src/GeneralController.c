/*
    NOTE
    DO I NEED TO VALIDATE THE PLAYER NUMBER WHEN RECEIVING PLAYER UPDATES AND TAGGING?
*/

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
#define BUFFSIZE        64
#define MIN_PLAYERS     2   // min players to trigger GAME_STATE_ACTIVE

extern int RUNNING;
double WIN_RATIO = MAX_OBJECTIVES * 0.75;

// Controllers
void* GeneralController(void* ipcSocks);
void connectionController(void* sockets, packet_t pType, PKT_PLAYERS_UPDATE *pLists, PKT_GAME_STATUS *gameInfo);
void lobbyController(void* sockets, PKT_PLAYERS_UPDATE *pLists, PKT_GAME_STATUS *gameInfo);
void runningController(void* sockets, PKT_PLAYERS_UPDATE *pLists, PKT_GAME_STATUS *gameInfo);
void endController(void* sockets, PKT_PLAYERS_UPDATE *pLists, PKT_GAME_STATUS *gameInfo);

// Helpers
size_t countObjectives(bool_t *objectives);
size_t countTeams(const teamNo_t *playerTeams, size_t *team1, size_t *team2);
void zeroPlayerLists(PKT_PLAYERS_UPDATE *pLists, const int maxPlayers);

// Game Utility
void balanceTeams(const teamNo_t *teamIn, teamNo_t *teamOut);
status_t getGameStatus(const status_t *playerStatus, const teamNo_t *playerTeams);
int setup(SOCKET in, int *maxPlayers, PKT_PLAYERS_UPDATE *pLists);
void forceMoveAll(void* sockets, PKT_PLAYERS_UPDATE *pLists, status_t status);

// Socket Utility
inline void writePacket(SOCKET sock, void* packet, packet_t type);
inline void writeIPC(SOCKET sock, void* buf, packet_t type);

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
void* GeneralController(void* ipcSocks)
{

    /*
        Packets 3 and 8 collectively almost all the data required by the general controller.
        The general controller will store them as a packet to keep information together.
    */
    struct pkt03 pInfoLists;
    struct pkt08 gameInfo;

    int maxPlayers;

    SOCKET ipc   = ((SOCKET*) ipcSocks)[0];     // Socket to relay IPC messages

    setup(ipc, &maxPlayers, &pInfoLists);
	DEBUG(DEBUG_INFO, "GC> Setup Complete");
	while (RUNNING) {

        lobbyController(ipcSocks, &pInfoLists,&gameInfo);
        runningController(ipcSocks, &pInfoLists, &gameInfo);
        endController(ipcSocks, &pInfoLists, &gameInfo);

	}
	return NULL;
}

void lobbyController(void* sockets, PKT_PLAYERS_UPDATE *pLists, PKT_GAME_STATUS *gameInfo)
{
	SOCKET net   = ((SOCKET*) sockets)[1];     // Socket to relay network messages
	teamNo_t desiredTeams[MAX_PLAYERS] = {0};

    gameInfo->game_status = GAME_STATE_WAITING;

	PKT_READY_STATUS    inPkt5;
	packet_t pType;

	while(gameInfo->game_status == GAME_STATE_WAITING)
    {
        if(!RUNNING)
            return;

        pType = getPacketType(net);
        switch(pType)
        {
        case IPC_PKT_1:
        case IPC_PKT_2:
            connectionController(sockets, pType, pLists, gameInfo);
        break;
        case 5:
            DEBUG(DEBUG_INFO, "GC> Lobby> Received pakcet 5");
            getPacket(net, &inPkt5, sizeof(netPacketSizes[5]));

            pLists->readystatus[inPkt5.player_number] = inPkt5.ready_status;
            strcpy(pLists->otherPlayers_name[inPkt5.player_number], inPkt5.player_name);
            desiredTeams[inPkt5.player_number] = inPkt5.team_number;

            gameInfo->game_status = getGameStatus(pLists->readystatus, desiredTeams);
            if(gameInfo->game_status == GAME_STATE_ACTIVE)
            {
                balanceTeams(desiredTeams, pLists->otherPlayers_teams);
                forceMoveAll(sockets, pLists, PLAYER_STATE_ACTIVE);
            }
            DEBUG(DEBUG_WARN, "GC> Lobby> All players ready and moved to floor 1");
            writePacket(net, pLists, 3);
        break;
        default:
            DEBUG(DEBUG_ALRM, "GC> Lobby> Receiving invalid packet");
        break;
        }
    }
}

void runningController(void* sockets, PKT_PLAYERS_UPDATE *pLists, PKT_GAME_STATUS *gameInfo)
{
	SOCKET net   = ((SOCKET*) sockets)[1];     // Socket to relay network messages

    PKT_GAME_STATUS inPkt8;
    PKT_TAGGING     inPkt14;
    PKT_FORCE_MOVE  outIPC3;

	packet_t pType;
    size_t team1 = 0, team2 = 0, objCount = 0;
	DEBUG(DEBUG_INFO, "GC> In runningController");
    while(gameInfo->game_status == GAME_STATE_ACTIVE)
    {
        if(!RUNNING) {
            return;
        }

        pType = getPacketType(net);
        switch(pType)
        {
        case IPC_PKT_1:
        case IPC_PKT_2:
            connectionController(sockets, pType, pLists, gameInfo);
        break;
        case 8:
            DEBUG(DEBUG_INFO, "GC> Running> Received packet 8");
            getPacket(net, &inPkt8, netPacketSizes[8]);

            memcpy(gameInfo->objectives_captured, &(inPkt8.objectives_captured), MAX_OBJECTIVES);

            // Check win
            objCount = countObjectives(inPkt8.objectives_captured);
            if((objCount/MAX_OBJECTIVES) > WIN_RATIO){
                gameInfo->game_status = GAME_TEAM2_WIN;
            } else {
                gameInfo->game_status = GAME_STATE_ACTIVE;
            }
            writePacket(net, gameInfo, 8);
        break;
        case 14:
            DEBUG(DEBUG_INFO, "GC> Running> Received packet 14");
            getPacket(net, &inPkt14, netPacketSizes[14]);

            outIPC3.playerNo = inPkt14.taggee_id;
            outIPC3.newFloor = FLOOR_LOBBY;
            writeIPC(net, &outIPC3, 0xB3);

            pLists->otherPlayers_teams[inPkt14.taggee_id] = TEAM_NONE;
            pLists->readystatus[inPkt14.taggee_id] = PLAYER_STATE_OUT;
            writePacket(net, pLists, 3);

            //Check win
            countTeams(pLists->otherPlayers_teams, &team1, &team2);
            if(team2 <= 0) {
                gameInfo->game_status = GAME_TEAM1_WIN;
                writePacket(net, gameInfo, 8);
            }

        break;
        default: DEBUG(DEBUG_ALRM, "GC> Running> Receiving invalid packet"); break;
        }
    }
}


void endController(void* sockets, PKT_PLAYERS_UPDATE *pLists, PKT_GAME_STATUS *gameInfo)
{
    SOCKET net   = ((SOCKET*) sockets)[1];     // Socket to relay network messages


    if(!RUNNING) {
        return;
    }

    forceMoveAll(sockets, pLists, PLAYER_STATE_WAITING);
    memset(pLists->otherPlayers_teams, TEAM_NONE, sizeof(teamNo_t)*MAX_PLAYERS);
    writePacket(net, pLists, 3);
}

void connectionController(void* sockets, packet_t pType, PKT_PLAYERS_UPDATE *pLists, PKT_GAME_STATUS *gameInfo)
{
    SOCKET net   = ((SOCKET*) sockets)[1];     // Socket to relay network messages

    // num players has no significance unless i count through the list
    //size_t numPlayers = 0;

    PKT_NEW_CLIENT  inIPC1;
    PKT_LOST_CLIENT inIPC2;

    switch(pType)
    {
    case IPC_PKT_1: // New Player
        DEBUG(DEBUG_INFO, "GC> Received IPC_PKT_1");
        getPacket(net, &inIPC1, ipcPacketSizes[1]);

//      numPlayers++;

            // Assign no team and send him to the lobby
            pLists->otherPlayers_teams[inIPC1.playerNo] = TEAM_NONE;
            strcpy(pLists->otherPlayers_name[inIPC1.playerNo], inIPC1.client_player_name);
            pLists->readystatus[inIPC1.playerNo] = PLAYER_STATE_WAITING;
            pLists->player_valid[inIPC1.playerNo] = TRUE;

            writePacket(net, pLists, 3);
            writePacket(net, gameInfo, 8);
        break;
		case IPC_PKT_2: // Player Lost -> Sends pkt 3 Players Update
			DEBUG(DEBUG_INFO, "GC> Received IPC_PKT_2");
//			if (numPlayers < 1)
//			{
//                DEBUG("GC> numPlayers < 1 HOW COULD WE LOSE SOMEONE?!");
//                if(pLists->player_valid[inIPC2.playerNo] == TRUE)
//                {
//                    DEBUG("GC> ...because the playerNo is still valid..BUT WHY!?");
//                }
//                break;
//			}

			getPacket(net, &inIPC2, ipcPacketSizes[2]);
			if(pLists->player_valid[inIPC2.playerNo] == FALSE)
			{
                DEBUG(DEBUG_WARN, "GC> Sources tell me this player is already not valid.. at least he's actrually gone now");
                break;
			}

//			numPlayers--;

            pLists->otherPlayers_name[inIPC2.playerNo][0] = '\0';
            pLists->otherPlayers_teams[inIPC2.playerNo] = TEAM_NONE;
            pLists->player_valid[inIPC2.playerNo] = FALSE;
            pLists->readystatus[inIPC2.playerNo] = PLAYER_STATE_DROPPED;
            writePacket(net, pLists, 3);

            //TO-DO check if that was the last player of a team and trigger a win condition
        break;    DEBUG(DEBUG_WARN, "GC> Lost player is not valid");
    default:
        DEBUG(DEBUG_ALRM, "GC> This should never be possible... gg");
    break;
    }
}

/***********************************************************/
/******                HELPER FUNCTIONS               ******/
/***********************************************************/
void forceMoveAll(void* sockets, PKT_PLAYERS_UPDATE *pLists, status_t status)
{
    int i;
    floorNo_t floor = FLOOR_LOBBY;
    PKT_FORCE_MOVE      outIPC3;
    SOCKET ipc   = ((SOCKET*) sockets)[0];     // Socket to relay network messages

    if(status == PLAYER_STATE_ACTIVE) {
        floor = 1;
    } else if(status == PLAYER_STATE_WAITING) {
        floor = FLOOR_LOBBY;
    }

    for(i = 0; pLists->player_valid[i] == FALSE; ++i)
    {
        pLists->readystatus[i] = status;
        outIPC3.playerNo = i;
        outIPC3.newFloor = floor;
        writeIPC(ipc, &outIPC3, 0xB3);
    }
}

size_t countObjectives(bool_t *objectives)
{
    int i, val = 0;
    for(i = 0; i < MAX_OBJECTIVES; ++i)
    {
        if(*(objectives+i) == 1)
            val++;
    }
    return val;
}

// params [out] team1 - number of players in team1
//        [out] team2 - number of players in team2
// returns total number of players in the game
size_t countTeams(const teamNo_t *playerTeams, size_t *team1, size_t *team2)
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
            default: DEBUG(DEBUG_ALRM, "GC> Error getting player's team"); break;
        }

    }
    return (*team1) + (*team2);
}

void balanceTeams(const teamNo_t *teamIn, teamNo_t *teamOut)
{
    size_t team1Count = 0, team2Count = 0, i = 0;
    memcpy(teamOut, teamIn, sizeof(teamNo_t)*MAX_PLAYERS);
    countTeams(teamIn, &team1Count, &team2Count);
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
status_t getGameStatus(const status_t *playerStatus, const teamNo_t *playerTeams)
{
    status_t s = GAME_STATE_WAITING;
    size_t i;
    int playerCount = 0;
    int readyCount = 0;

    for(i = 0; i < MAX_PLAYERS; i++)
    {
        if(*(playerStatus+i) != PLAYER_STATE_INVALID){
            playerCount++;
        }

        if(!(playerStatus+i) == PLAYER_STATE_READY){
            readyCount++;
        }
    }

    if(playerCount >= MIN_PLAYERS && readyCount == playerCount){
        s = GAME_STATE_ACTIVE;
    }
    return s;
}


void zeroPlayerLists(PKT_PLAYERS_UPDATE *pLists, const int maxPlayers)
{
    int i, j;
    for(i = 0; i < MAX_PLAYERS; ++i)
    {
        pLists->player_valid[i] = FALSE;
        for(j = 0; j < MAX_NAME; ++j) {
            pLists->otherPlayers_name[i][j] = '\0';
        }

        if(i < maxPlayers){
            pLists->player_valid[i] = TRUE;
            pLists->readystatus[i] = PLAYER_STATE_AVAILABLE;
        } else if( i > maxPlayers){
            pLists->readystatus[i] = PLAYER_STATE_INVALID;
        }
	}
}


// receives etup packet and initializes player lists
// returns 1 on success <0 on error
int setup(SOCKET in, int *maxPlayers, PKT_PLAYERS_UPDATE *pLists)
{
    char msg[BUFFSIZE] = {0};
    struct pktB0 pkt0;
    int pType;

    pType = getPacketType(in);

    if(pType != IPC_PKT_0) {
        sprintf(msg, "GC> Expected %d - Received %d", IPC_PKT_0, pType);
        DEBUG(DEBUG_ALRM, msg);
        return -1;
    }

    getPacket(in, &pkt0, ipcPacketSizes[0]);
    *maxPlayers = pkt0.maxPlayers;

    zeroPlayerLists(pLists, *maxPlayers);
	return 1;
}

/***********************************************************/
/******         SOCKET UTILITY FUNCTIONS              ******/
/***********************************************************/
inline void writeIPC(SOCKET sock, void* buf, packet_t type)
{
    packet_t outType = type - 0xB0;
    write(sock, &outType, sizeof(packet_t));
    write(sock, buf, ipcPacketSizes[outType]);

    #if DEBUG_ON
        char buff[BUFFSIZE];
        sprintf(buff, "GC> Sent packet: %d", type);
        DEBUG(DEBUG_INFO, buff);
    #endif
}

inline void writePacket(SOCKET sock, void* packet, packet_t type)
{
	OUTMASK m;
	OUT_SETALL(m);

	write(sock, &type, sizeof(packet_t));
    write(sock, packet, netPacketSizes[type]);
    write(sock, &m, sizeof(OUTMASK));

    #if DEBUG_ON
        char buff[BUFFSIZE];
        sprintf(buff, "GC> Sent packet: %d", type);
        DEBUG(DEBUG_INFO, buff);
    #endif
}
