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
#define MIN_PLAYER_TEAM 1   // for mercy rule? if too many people

extern int RUNNING;
double WIN_RATIO = MAX_OBJECTIVES * 0.75;

// Controllers
void* GeneralController(void* ipcSocks);
void ongoingController(void* sockets, packet_t pType, PKT_PLAYERS_UPDATE *pLists, PKT_GAME_STATUS *gameInfo);
void lobbyController(void* sockets, PKT_PLAYERS_UPDATE *pLists, PKT_GAME_STATUS *gameInfo);
void runningController(void* sockets, PKT_PLAYERS_UPDATE *pLists, PKT_GAME_STATUS *gameInfo);
void endController(void* sockets, PKT_PLAYERS_UPDATE *pLists, PKT_GAME_STATUS *gameInfo);

// Helpers
size_t countObjectives(bool_t *objectives);
size_t countTeams(const teamNo_t *playerTeams, size_t *team1, size_t *team2);
size_t countActivePlayers(const teamNo_t *playerTeams);
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

// handles new player and player lost packets
// checks win on player lost
// should be called in each controller that is allowed to accept these packets...pretty much all of them
// created march 25
// march 26 added player selection
void ongoingController(void* sockets, packet_t pType, PKT_PLAYERS_UPDATE *pLists, PKT_GAME_STATUS *gameInfo)
{
    SOCKET in   = ((SOCKET*) sockets)[0];     // Socket to relay network messages
    SOCKET out   = ((SOCKET*) sockets)[1];     // Socket to relay network messages

//    size_t numPlayers = countActivePlayers(pLists->playerTeams);


    PKT_NEW_CLIENT  inIPC1;
    PKT_LOST_CLIENT inIPC2;
    PKT_CHAT pktchat;
    PKT_SPECIAL_TILE pktTile;

    switch(pType)
    {
    case IPC_PKT_1: // New Player
        DEBUG(DEBUG_INFO, "GC> Received IPC_PKT_1");
        getPacket(in, &inIPC1, ipcPacketSizes[1]);

  //      numPlayers++;

        // Assign no team and send him to the lobby
        pLists->playerTeams[inIPC1.playerNo] = TEAM_NONE;
        strcpy(pLists->playerNames[inIPC1.playerNo], inIPC1.playerName);
        pLists->readystatus[inIPC1.playerNo] = PLAYER_STATE_WAITING;
        pLists->playerValid[inIPC1.playerNo] = TRUE;
        pLists->characters[inIPC1.playerNo] = inIPC1.character;

        writePacket(out, pLists, 3);
        writePacket(out, gameInfo, 8);
        break;
		case IPC_PKT_2: // Player Lost -> Sends pkt 3 Players Update
			DEBUG(DEBUG_INFO, "GC> Received IPC_PKT_2");
//			if (numPlayers < 1)
//			{
//                DEBUG(DEBUG_ALRM, "GC> numPlayers < 1 HOW COULD WE LOSE SOMEONE?!");
//                if(pLists->playerValid[inIPC2.playerNo] == TRUE)
//                {
//                    DEBUG(DEBUG_WARN, "GC> ...because the playerNo is still valid..BUT WHY!?");
//                }
//                break;ff
//			}

			getPacket(in, &inIPC2, ipcPacketSizes[2]);
			if(pLists->playerValid[inIPC2.playerNo] == FALSE)
			{
                DEBUG(DEBUG_WARN, "GC> Sources tell me this player is already not valid.. at least he's actrually gone now");
                break;
			}

//			numPlayers--;

            pLists->playerNames[inIPC2.playerNo][0] = '\0';
            pLists->playerTeams[inIPC2.playerNo] = TEAM_NONE;
            pLists->playerValid[inIPC2.playerNo] = FALSE;
            pLists->readystatus[inIPC2.playerNo] = PLAYER_STATE_DROPPED;
            writePacket(out, pLists, 3);

            //TO-DO check if that was the last player of a team and trigger a win condition
            DEBUG(DEBUG_WARN, "GC> Player removed");
            break;

        case 4: // chat
            getPacket(in, &pktchat, netPacketSizes[4]);
            sendChat(&pktchat, pLists->playerTeams, out);
            break;

        case 6: // special tile placed
            getPacket(in, &pktTile, netPacketSizes[6]);
            writePacket(out, pktTile, 6);
            break;
    default:
        DEBUG(DEBUG_ALRM, "GC> This should never be possible... gg");
        break;
    }
}

void lobbyController(void* sockets, PKT_PLAYERS_UPDATE *pLists, PKT_GAME_STATUS *gameInfo)
{
    SOCKET in   = ((SOCKET*) sockets)[0];     // Socket to relay network messages
	SOCKET out  = ((SOCKET*) sockets)[1];     // Socket to relay network messages
	teamNo_t desiredTeams[MAX_PLAYERS] = {0};

	PKT_READY_STATUS    inPkt5;
	packet_t pType;

    gameInfo->game_status = GAME_STATE_WAITING;
    memset(pLists->playerTeams, TEAM_NONE, sizeof(teamNo_t)*MAX_PLAYERS);

	while(gameInfo->game_status == GAME_STATE_WAITING)
    {
        if(!RUNNING)
            return;

        pType = getPacketType(in);
        switch(pType)
        {
        case IPC_PKT_1:
        case IPC_PKT_2:
        case 4:
            ongoingController(sockets, pType, pLists, gameInfo);
        break;
        case 5:
            DEBUG(DEBUG_INFO, "GC> Lobby> Received pakcet 5");
            getPacket(in, &inPkt5, netPacketSizes[5]);

            pLists->readystatus[inPkt5.playerNumber] = inPkt5.ready_status;
            strcpy(pLists->playerNames[inPkt5.playerNumber], inPkt5.playerName);
            desiredTeams[inPkt5.playerNumber] = inPkt5.team_number;

            gameInfo->game_status = getGameStatus(pLists->readystatus, desiredTeams);
            if(gameInfo->game_status == GAME_STATE_ACTIVE)
            {
                balanceTeams(desiredTeams, pLists->playerTeams);
                forceMoveAll(sockets, pLists, PLAYER_STATE_ACTIVE);
                DEBUG(DEBUG_WARN, "GC> Lobby> All players ready and moved to floor 1");
            }

            writePacket(out, pLists, 3);
        break;
        default:
            DEBUG(DEBUG_ALRM, "GC> Lobby> Receiving invalid packet");
        break;
        }
    }
}

// main game engine controller
// refactored march 25
// created WAY BACK IN THE DAY
//
void runningController(void* sockets, PKT_PLAYERS_UPDATE *pLists, PKT_GAME_STATUS *gameInfo)
{
    SOCKET in   = ((SOCKET*) sockets)[0];     // Socket to relay network messages
	SOCKET out   = ((SOCKET*) sockets)[1];     // Socket to relay network messages

    PKT_GAME_STATUS inPkt8;
    PKT_TAGGING     inPkt14;
    PKT_FORCE_MOVE  outIPC3;

	packet_t pType;
    size_t team1 = 0, team2 = 0, objCount = 0;
	DEBUG(DEBUG_INFO, "GC> In runningController");
	writePacket(out, gameInfo, 8);
	chatGameStart();
    while(gameInfo->game_status == GAME_STATE_ACTIVE)
    {
        if(!RUNNING) {
            return;
        }

        pType = getPacketType(in);
        switch(pType)
        {        case IPC_PKT_1:
        case IPC_PKT_2:
        case 4:
            ongoingController(sockets, pType, pLists, gameInfo);
        break;
        case 8:
            DEBUG(DEBUG_INFO, "GC> Running> Received packet 8");
            getPacket(in, &inPkt8, netPacketSizes[8]);

            memcpy(gameInfo->objectiveStates, &(inPkt8.objectiveStates), MAX_OBJECTIVES);

            // Check win
            objCount = countObjectives(inPkt8.objectiveStates);
            if((objCount/MAX_OBJECTIVES) >= WIN_RATIO){
                gameInfo->game_status = GAME_TEAM2_WIN;
            } else {
                gameInfo->game_status = GAME_STATE_ACTIVE;
            }
            writePacket(out, gameInfo, 8);
        break;
        case 14:
            DEBUG(DEBUG_INFO, "GC> Running> Received packet 14");
            getPacket(in, &inPkt14, netPacketSizes[14]);

            outIPC3.playerNo = inPkt14.taggee_id;
            outIPC3.newFloor = FLOOR_LOBBY;
            writeIPC(out, &outIPC3, IPC_PKT_3);

            pLists->playerTeams[inPkt14.taggee_id] = TEAM_NONE;
            pLists->readystatus[inPkt14.taggee_id] = PLAYER_STATE_OUT;
            writePacket(out, pLists, 3);

            //Check win
            countTeams(pLists->playerTeams, &team1, &team2);
            if(team2 <= 0) {
                gameInfo->game_status = GAME_TEAM1_WIN;
                writePacket(out, gameInfo, 8);
            }

        break;
        default: DEBUG(DEBUG_ALRM, "GC> Running> Receiving invalid packet"); break;
        }
    }
}

// moves all players to lobby
// created march 24
// revised march 25 lobby controller strips player teams
void endController(void* sockets, PKT_PLAYERS_UPDATE *pLists, PKT_GAME_STATUS *gameInfo)
{
    SOCKET out   = ((SOCKET*) sockets)[1];     // Socket to relay network messages


    if(!RUNNING) {
        return;
    }

    forceMoveAll(sockets, pLists, PLAYER_STATE_WAITING);
    // players are stripped of their teams when entering the lobby controller
    // memset(pLists->playerTeams, TEAM_NONE, sizeof(teamNo_t)*MAX_PLAYERS);
    writePacket(out, pLists, 3);
}

/***********************************************************/
/******                HELPER FUNCTIONS               ******/
/***********************************************************/

// created march 24
// sends ipc3 for all players
// moves all players to specified floor and sets their specified state
void forceMoveAll(void* sockets, PKT_PLAYERS_UPDATE *pLists, status_t status)
{
    int i;
    floorNo_t floor = FLOOR_LOBBY;
    PKT_FORCE_MOVE      outIPC3;
    SOCKET in   = ((SOCKET*) sockets)[0];     // Socket to relay ipc messages

    // Game is going active
    if(status == PLAYER_STATE_ACTIVE){
        for(i = 0; i < MAX_PLAYERS; ++i){
            if(pLists->playerTeams[i] == TEAM_COPS){
                floor = 3;
            }
            else if(pLists->playerTeams[i] == TEAM_ROBBERS){
                floor = 1;
            }

            if(pLists->playerTeams[i] != TEAM_NONE){
                outIPC3.playerNo = i;
                outIPC3.newFloor = floor;
                writeIPC(in, &outIPC3, IPC_PKT_3);
            }
        }
    }

    // Game is ending
    if(status == PLAYER_STATE_WAITING){
        for(i = 0; i < MAX_PLAYERS; ++i){

            if(pLists->playerTeams[i] != TEAM_NONE){
                outIPC3.playerNo = i;
                outIPC3.newFloor = FLOOR_LOBBY;
                writeIPC(in, &outIPC3, IPC_PKT_3);
            }
        }
    }

}

// counts the captured objectives
// created march 14
// refactored march 24
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

//created march 25
//uses player teams to count the number of players still in the game
// returns the amoun of players
size_t countActivePlayers(const teamNo_t *playerTeams)
{
    size_t team1 = 0, team2 = 0;
    return countTeams(playerTeams, &team1, &team2);
}

// params [out] team1 - number of players in team1
//        [out] team2 - number of players in team2
// returns total number of players in the game
// created march 12
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

// in - teams chosed by the users
// out- teams balanced by the server
// created march 12
void balanceTeams(const teamNo_t *teamIn, teamNo_t *teamOut)
{
    size_t team1Count = 0, team2Count = 0, i = 0;
    memcpy(teamOut, teamIn, sizeof(teamNo_t)*MAX_PLAYERS);
    countTeams(teamIn, &team1Count, &team2Count);
    while(team1Count < team2Count)
    {
        if(*(teamOut+i) == TEAM_ROBBERS)
        {
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
        if(*(playerStatus+i) == PLAYER_STATE_WAITING || *(playerStatus+i) == PLAYER_STATE_READY){
            playerCount++;
        }

        if(*(playerStatus+i) == PLAYER_STATE_READY){
            readyCount++;
        }
    }

    if(playerCount >= MIN_PLAYERS && readyCount == playerCount){
        s = GAME_STATE_ACTIVE;
    }
    return s;
}

// useful in setup of a new game
// sets ll player data to initial states
// march25 created
void zeroPlayerLists(PKT_PLAYERS_UPDATE *pLists, const int maxPlayers)
{
    int i, j;
    for(i = 0; i < MAX_PLAYERS; ++i)
    {
        pLists->playerValid[i] = FALSE;
        pLists->characters[i] = 0;
        for(j = 0; j < MAX_NAME; ++j) {
            pLists->playerNames[i][j] = '\0';
        }

        if(i < maxPlayers){
            pLists->readystatus[i] = PLAYER_STATE_AVAILABLE;

        } else if( i >= maxPlayers){
            pLists->readystatus[i] = PLAYER_STATE_INVALID;
        }
	}
}


// receives setup packet and initializes player lists
// returns 1 on success <0 on error
// march 25 created
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

    write(sock, &type, sizeof(packet_t));
    type =- 0xB0;
    write(sock, buf, ipcPacketSizes[type]);

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
