/** @ingroup Server */
/** @{ */

/**
 * This file contains all methods responsible for "general" interactions between server and client.
 * This controller is in charge of setting the game winner and maintaining the list of objectives.
 * It also stores all of the player information and keeps it updated as connections are dropped or
 * accepted.
 *
 *
 * @file GeneralController.c
 */

/** @} */

#include "Server.h"
#define BUFFSIZE        64

extern int RUNNING;

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
void clearUnexpectedPacket(SOCKET sock, packet_t type);

// Game Utility
void balanceTeams(teamNo_t *teamIn, teamNo_t *teamOut);
status_t getGameStatus(const status_t *playerStatus, const teamNo_t *playerTeams);
int setup(SOCKET in, int *maxPlayers, PKT_PLAYERS_UPDATE *pLists);
void forceMoveAll(void* sockets, PKT_PLAYERS_UPDATE *pLists, status_t status);

// Socket Utility
inline void writePacket(SOCKET sock, void* packet, packet_t type);
inline void writePacketTo(SOCKET sock, void* packet, packet_t type, OUTMASK mask);
inline void writeIPC(SOCKET sock, void* buf, packet_t type);

// Desired teams are maintainted accross all states
teamNo_t desiredTeams[MAX_PLAYERS] = {0};

char serverName[MAX_NAME] = {0};

/**
 * Ongoing function while the server is running. Stores all player and game objective information.
 * Controls packets via the sub controllers.
 *
 * Revisions:
 *      -# March 26 - Largely refactored to have subcontrollers.
 *
 * @param[in]   ipcSocks     Array of 2 sockets for reading and writing.
 * @return void
 *
 * @designer German Villarreal
 * @author German Villarreal
 *
 * @date February 20, 2014
 */
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

    bzero(&pInfoLists, netPacketSizes[3]);
    bzero(&gameInfo, netPacketSizes[8]);

    setup(ipc, &maxPlayers, &pInfoLists);
	DEBUG(DEBUG_INFO, "GC> Setup Complete");
	while (RUNNING) {

        lobbyController(ipcSocks, &pInfoLists,&gameInfo);
        runningController(ipcSocks, &pInfoLists, &gameInfo);
        endController(ipcSocks, &pInfoLists, &gameInfo);

	}
	return NULL;
}

/**
 * Handles packets that may be received regardless of the game state. This packet should be called
 * in both the lobby and running controller to allow it to process these incoming packets.
 *
 * Communication:
 *  - Listens for IPC_PKT_1 (New Player) -> Sends pkt03 (PKT_PLAYERS_UPDATE)
 *      Adds the player to player lists.
 *  - IPC_PKT_2 (Player Lost) -> pkt03 (PKT_PLAYERS_UPDATE)
 *      Removes the player information creating an available slot. Verifies whether enough players are
 *      still connected. Sends appropriate team win or updated player list.
 *  - pkt04 (PKT_CHAT) -> pkt04 (PKT_CHAT)
 *      Forwards the packet to appropriate players
 *
 * Revisions:
 *      -# March 26 - Store desired character.
 *
 * @param[in]   sockets     Array of 2 sockets for reading and writing.
 * @param[in]  pType        The packet type read that requires processing.
 * @param[out]  pLists      Holds all information related to each player.
 * @param[out]  gameInfo    Holds all information related to game state and objectives.
 * @return void
 *
 * @designer German Villarreal
 * @author German Villarreal
 *
 * @date March 25, 2014
 */
void ongoingController(void* sockets, packet_t pType, PKT_PLAYERS_UPDATE *pLists, PKT_GAME_STATUS *gameInfo)
{
    SOCKET in   = ((SOCKET*) sockets)[0];     // Socket to relay network messages
    SOCKET out   = ((SOCKET*) sockets)[1];     // Socket to relay network messages

    size_t team1Count = 0, team2Count = 0;

    PKT_NEW_CLIENT  inIPC1;
    PKT_LOST_CLIENT inIPC2;
    PKT_CHAT pktchat;
    PKT_SPECIAL_TILE pktTile;

    int i = 0;
    OUTMASK m;

    bzero(&inIPC1, ipcPacketSizes[1]);
    bzero(&inIPC2, ipcPacketSizes[2]);
    bzero(&pktchat, netPacketSizes[4]);
    bzero(&pktTile, netPacketSizes[6]);

    switch(pType)
    {
        case IPC_PKT_1: // New Player
            DEBUG(DEBUG_INFO, "GC> Received IPC_PKT_1");
            getPacket(in, &inIPC1, ipcPacketSizes[1]);

            // Assign no team and send him to the lobby
            pLists->playerTeams[inIPC1.playerNo] = TEAM_NONE;
            memcpy(pLists->playerNames[inIPC1.playerNo], inIPC1.playerName, MAX_NAME);
            pLists->readystatus[inIPC1.playerNo] = PLAYER_STATE_WAITING;
            pLists->playerValid[inIPC1.playerNo] = TRUE;
            pLists->characters[inIPC1.playerNo] = inIPC1.character;

            // Join message
            printf("%s [%d] has joined the game.\n", pLists->playerNames[inIPC1.playerNo], inIPC1.playerNo);
            if(SERVER_MESSAGES){
                bzero(pktchat.message, MAX_MESSAGE);
                sprintf(pktchat.message, "%s has joined the game.", pLists->playerNames[inIPC1.playerNo]);
                pktchat.sendingPlayer = MAX_PLAYERS;
                sendChat(&pktchat, pLists->playerTeams, out);
                bzero(pktchat.message, MAX_MESSAGE);
            }

            writePacket(out, pLists, 3);

            if(SERVER_MESSAGES){
                OUT_ZERO(m);
                OUT_SET(m, inIPC1.playerNo);
                bzero(pktchat.message, MAX_MESSAGE);
                pktchat.sendingPlayer = MAX_PLAYERS;
                sprintf(pktchat.message, "Welcome to the server %s [v%.1lf]", serverName, SERVER_VERSION);
                writePacketTo(out, &pktchat, 4, m);
            }

            break;
		case IPC_PKT_2: // Player Lost -> Sends pkt 3 Players Update
			DEBUG(DEBUG_INFO, "GC> Received IPC_PKT_2");

			getPacket(in, &inIPC2, ipcPacketSizes[2]);

            // leave message
            printf("%s [%d] has left the game.\n", pLists->playerNames[inIPC2.playerNo], inIPC2.playerNo);

            if(SERVER_MESSAGES){
                bzero(pktchat.message, MAX_MESSAGE);
                sprintf(pktchat.message, "%s has left the game.", pLists->playerNames[inIPC2.playerNo]);
                pktchat.sendingPlayer = MAX_PLAYERS;
                sendChat(&pktchat, pLists->playerTeams, out);
                bzero(pktchat.message, MAX_MESSAGE);
            }

            bzero(pLists->playerNames[inIPC2.playerNo], MAX_NAME);
            pLists->playerTeams[inIPC2.playerNo] = TEAM_NONE;
            pLists->playerValid[inIPC2.playerNo] = FALSE;
            pLists->readystatus[inIPC2.playerNo] = PLAYER_STATE_DROPPED;
            desiredTeams[inIPC2.playerNo] = TEAM_NONE;
            DEBUG(DEBUG_WARN, "GC> Player removed");

            if(gameInfo->game_status == GAME_STATE_ACTIVE)
            {
                countTeams(pLists->playerTeams, &team1Count, &team2Count);
                if(team1Count == 0) {
                    gameInfo->game_status = GAME_TEAM2_WIN;
                    printf("Guards weren't getting paid enough.\n");
                    DEBUG(DEBUG_INFO, "GC> Team 2 (Hackers) won - no cops left");
                }
                else if(team2Count == 0) {
                    gameInfo->game_status = GAME_TEAM1_WIN;
                    printf("Hackers Eliminated!\n");
                    DEBUG(DEBUG_INFO, "GC> Team 1 (Guards) won - no robbers left");
                }
                else{
                    writePacket(out, pLists, 3);
                }
            }
            else{
                writePacket(out, pLists, 3);
            }
            break;

        case 4: // chat
            getPacket(in, &pktchat, netPacketSizes[4]);
            if(PRINT_CHAT_TO_SERVER){
                printf("%s: \"%s\"\n", ((pktchat.sendingPlayer == MAX_PLAYERS) ? "Server" : pLists->playerNames[pktchat.sendingPlayer]), pktchat.message);
            }
            sendChat(&pktchat, pLists->playerTeams, out);
            break;

        case 6: // special tile placed
            getPacket(in, &pktTile, netPacketSizes[6]);
            OUT_ZERO(m);
            for(i = 0; i < MAX_PLAYERS; ++i){
                if(i != pktTile.placingPlayer){
                    OUT_SET(m, i);
                }
            }
            writePacketTo(out, &pktTile, 6, m);
            break;
        default:
            DEBUG(DEBUG_ALRM, "GC> This should never be possible... gg");
            break;
    }
}

/**
 * Handles packets that may be received while the game is in the lobby. Players can move around, pick
 * teams, and chat in the lobby.
 *
 * Communication:
 *  - Listens for pkt05 (PKT_READY_STATUS)-> Sends pkt03 (PKT_PLAYERS_UPDATE)
 *      Reads player states and desired teams. Once enough players are ready, the teams are checked and balanced
 *      if neccessary. The game state is then changed and players are sent to their respective floor depending on their
 *      new balanced teams.
 *
 * @param[in]   sockets     Array of 2 sockets for reading and writing.
 * @param[in]   pType        The packet type read that requires processing.
 * @param[out]  pLists      Holds all information related to each player.
 * @param[out]  gameInfo    Holds all information related to game state and objectives.
 * @return void
 *
 * @designer German Villarreal
 * @author German Villarreal
 *
 * @date March 25, 2014
 */
void lobbyController(void* sockets, PKT_PLAYERS_UPDATE *pLists, PKT_GAME_STATUS *gameInfo)
{
    SOCKET in   = ((SOCKET*) sockets)[0];     // Socket to relay network messages
	SOCKET out  = ((SOCKET*) sockets)[1];     // Socket to relay network messages

	PKT_READY_STATUS    inPkt5;
	packet_t pType;
	PKT_CHAT serverChat;

	int i;

	bzero(&inPkt5, netPacketSizes[5]);
	bzero(serverChat.message, MAX_MESSAGE);
	serverChat.sendingPlayer = MAX_PLAYERS;

    gameInfo->game_status = GAME_STATE_WAITING;
    memset(pLists->playerTeams, TEAM_NONE, sizeof(teamNo_t)*MAX_PLAYERS);

    for(i = 0; i < MAX_OBJECTIVES; ++i){
        gameInfo->objectiveStates[i] = OBJECTIVE_INVALID;
    }
    gameInfo->game_status = GAME_STATE_WAITING;

    printf("-- Lobby --\n");

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

            if(!pLists->playerValid[inPkt5.playerNumber]){
                break;
            }

            pLists->readystatus[inPkt5.playerNumber] = inPkt5.ready_status;
            memcpy(pLists->playerNames[inPkt5.playerNumber], inPkt5.playerName, MAX_NAME);
            pLists->playerNames[inPkt5.playerNumber][MAX_NAME - 1] = 0;
            desiredTeams[inPkt5.playerNumber] = inPkt5.team_number;

            gameInfo->game_status = getGameStatus(pLists->readystatus, desiredTeams);
            if(gameInfo->game_status == GAME_STATE_ACTIVE)
            {

                if(SERVER_MESSAGES){
                    i = COUNTDOWN_TIME;
                    bzero(serverChat.message, MAX_MESSAGE);
                    sprintf(serverChat.message, "Game Start in %d...", i);
                    sendChat(&serverChat, NULL, out);
                    sleep(1);
                    for(i = COUNTDOWN_TIME - 1; i > 0; --i){
                        bzero(serverChat.message, MAX_MESSAGE);
                        sprintf(serverChat.message, "%d...", i);
                        sendChat(&serverChat, NULL, out);
                        sleep(1);
                    }
                }

                balanceTeams(desiredTeams, pLists->playerTeams);
                forceMoveAll(sockets, pLists, PLAYER_STATE_ACTIVE);
                DEBUG(DEBUG_WARN, "GC> Lobby> All players ready and moved to floor 1");

                writePacket(out, pLists, 3);

                printf("Game Start!\n");
                printf("*********************************\n");
                printf("Hackers:\n");
                for(i = 0; i < MAX_PLAYERS; ++i){
                    if(pLists->playerTeams[i] == TEAM_ROBBERS){
                        printf(" - %s [%d]\n", pLists->playerNames[i], i);
                    }
                }

                printf("Guards:\n");
                for(i = 0; i < MAX_PLAYERS; ++i){
                    if(pLists->playerTeams[i] == TEAM_COPS){
                        printf(" - %s [%d]\n", pLists->playerNames[i], i);
                    }
                }
            }

        break;
        default:
            DEBUG(DEBUG_ALRM, "GC> Lobby> Receiving invalid packet");
            clearUnexpectedPacket(in, pType);
        break;
        }
    }
}

/**
 * Handles packets that may be received during regular gameplay. Runs for the duration of the game.
 *
 * Communication:
 *  - Listens for pkt05 (PKT_READY_STATUS) -> none.
 *      Allows to accept these packets in order to allow players to join the lobby while the game is running
 *      but it does not allow any other states to change. This allows players to ready up for the next game.
 *  - pkt06 (PKT_SPECIAL_TILE) -> pkt06 (PKT_SPECIAL_TILE)
 *      Reads when a player drops a special tile and echoes it to all other players.
 *  - pkt08 (PKT_GAME_STATUS) -> pkt08 (PKT_GAME_STATUS)
 *      Copies any newly captured objective and checks if enough were captured for them to win; echoes new
 *      list of objectives to all players.
 *  - pkt14 (PKT_TAGGING) -> pkt03 (PKT_PLAYERS_UPDATE) && pkt08 (PKT_GAME_STATUS)
 *      Sends the tagged player to the lobby, notifies all other players who was tagged and checks whether
 *      cops won the game.
 *
 *
 * @param[in]   sockets     Array of 2 sockets for reading and writing.
 * @param[out]  pLists      Holds all information related to each player.
 * @param[out]  gameInfo    Holds all information related to game state and objectives.
 * @return void
 *
 * @designer German Villarreal
 * @author German Villarreal
 *
 * @date March 25, 2014
 */
void runningController(void* sockets, PKT_PLAYERS_UPDATE *pLists, PKT_GAME_STATUS *gameInfo)
{
    SOCKET in   = ((SOCKET*) sockets)[0];     // Socket to relay network messages
	SOCKET out   = ((SOCKET*) sockets)[1];     // Socket to relay network messages

    PKT_READY_STATUS inPkt5;
    PKT_GAME_STATUS inPkt8;
    PKT_TAGGING     inPkt14;
    PKT_FORCE_MOVE  outIPC3;
    PKT_SPECIAL_TILE    inTile;
    PKT_CHAT serverChat;

    OUTMASK m;

    int i, j;
    int capCount[MAX_PLAYERS] = {0};
	packet_t pType;
	int objCount = 0, winCount = 0, curCount = 0;
    size_t team1 = 0, team2 = 0, totalPlayers = 0;

    // Zero out starting memeory
    bzero(&inPkt14, netPacketSizes[14]);
    bzero(&inPkt8, netPacketSizes[8]);
    bzero(&inPkt5, netPacketSizes[5]);
    bzero(&serverChat, netPacketSizes[4]);

    serverChat.sendingPlayer = MAX_PLAYERS;

    totalPlayers = countActivePlayers(pLists->playerTeams);

    // start with half the player count
    objCount = totalPlayers;
    // at least 3 floors
    objCount = (objCount < 12) ? 12 : objCount;
    // rounded to the nearest full floor
    objCount += objCount % 4;

    winCount = WIN_RATIO * objCount;

    for(i = 0; i < objCount; ++i){
        gameInfo->objectiveStates[i] = OBJECTIVE_AVAILABLE;
    }
    for(j = i; j < MAX_OBJECTIVES; ++j){
        gameInfo->objectiveStates[j] = OBJECTIVE_INVALID;
    }

    writePacket(out, gameInfo, 8);

    printf("There are %d objectives over %d floors.\nCapture %d to win!\n", objCount, objCount / 4, winCount);

    // Start of game message
    if(SERVER_MESSAGES){
        sleep(2);
        bzero(serverChat.message, MAX_MESSAGE);
        serverChat.sendingPlayer = MAX_PLAYERS;
        sprintf(serverChat.message, "There are %d objectives over %d floors.", objCount, objCount / 4);
        sendChat(&serverChat, NULL, out);
        sleep(1);
        bzero(serverChat.message, MAX_MESSAGE);
        sprintf(serverChat.message, "Capture %d to win!", winCount);
        sendChat(&serverChat, NULL, out);
        bzero(serverChat.message, MAX_MESSAGE);
    }

    printf("-- In Game --\n");

	DEBUG(DEBUG_INFO, "GC> In runningController");
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

        case 5:
            // Accept the packet 5, but don't allow game state to change
            getPacket(in, &inPkt5, netPacketSizes[5]);

            if(!pLists->playerValid[inPkt5.playerNumber]){
                break;
            }

            if(pLists->playerTeams[inPkt5.playerNumber] != TEAM_NONE){
                break;
            }
            pLists->readystatus[inPkt5.playerNumber] = inPkt5.ready_status;
            memcpy(pLists->playerNames[inPkt5.playerNumber], inPkt5.playerName, MAX_NAME);
            pLists->playerNames[inPkt5.playerNumber][MAX_NAME - 1] = 0;
            desiredTeams[inPkt5.playerNumber] = inPkt5.team_number;
            break;

        case 6:
            // Echo a special tile
            getPacket(in, &inTile, netPacketSizes[6]);
            OUT_ZERO(m);
            for(i = 0; i < MAX_PLAYERS; ++i){
                if(i != inTile.placingPlayer){
                    OUT_SET(m,i);
                }
            }
            writePacketTo(out, &inTile, 6, m);
            break;

        case 8:
            DEBUG(DEBUG_INFO, "GC> Running> Received packet 8");
            getPacket(in, &inPkt8, netPacketSizes[8]);

            // Update any new captures
            for(i = 0; i < objCount; ++i){
                if(inPkt8.objectiveStates[i] == OBJECTIVE_CAPTURED){
                    if(gameInfo->objectiveStates[i] != OBJECTIVE_CAPTURED){
                        gameInfo->objectiveStates[i] = OBJECTIVE_CAPTURED;
                        printf("Objective %d has been captured!\n", i);
                        if(SERVER_MESSAGES){
                            bzero(serverChat.message, MAX_MESSAGE);
                            serverChat.sendingPlayer = MAX_PLAYERS;
                            sprintf(serverChat.message, "** An objective has been compromised on floor %d! **", (i / 4 )+ 1);
                            sendChat(&serverChat, NULL, out);
                        }
                    }
                }
            }

            // Check win
            curCount = countObjectives(gameInfo->objectiveStates);
            if(curCount >= winCount){
                printf("All Objectives Captured!\n");
                gameInfo->game_status = GAME_TEAM2_WIN;
            } else {
                if(SERVER_MESSAGES){
                    bzero(serverChat.message, MAX_MESSAGE);
                    serverChat.sendingPlayer = MAX_PLAYERS;
                    sprintf(serverChat.message, "** %d remain! **", winCount - curCount);
                    sendChat(&serverChat, NULL, out);
                }
                gameInfo->game_status = GAME_STATE_ACTIVE;
                writePacket(out, gameInfo, 8);
            }
            break;
        case 14:
            DEBUG(DEBUG_INFO, "GC> Running> Received packet 14");
            getPacket(in, &inPkt14, netPacketSizes[14]);

            if(inPkt14.tagger_id >= MAX_PLAYERS || inPkt14.taggee_id >= MAX_PLAYERS){
                DEBUG(DEBUG_WARN, "Invalid tag packet");
                break;
            }

            if(pLists->playerTeams[inPkt14.taggee_id] != TEAM_ROBBERS){
                DEBUG(DEBUG_WARN, "Capturing non-robber prohibited");
                printf("Non robber cap: %d\n", inPkt14.taggee_id);
                break;
            }

            outIPC3.playerNo = inPkt14.taggee_id;
            outIPC3.newFloor = FLOOR_LOBBY;
            writeIPC(in, &outIPC3, IPC_PKT_3);

            printf("%s [%d] captured robber %s [%d]\n", pLists->playerNames[inPkt14.tagger_id],
                   inPkt14.tagger_id, pLists->playerNames[inPkt14.taggee_id], inPkt14.taggee_id);
            capCount[inPkt14.tagger_id]++;

            if(SERVER_MESSAGES){
                bzero(serverChat.message, MAX_MESSAGE);
                serverChat.sendingPlayer = MAX_PLAYERS;
                sprintf(serverChat.message, "** %s captured robber %s! **\n", pLists->playerNames[inPkt14.tagger_id], pLists->playerNames[inPkt14.taggee_id]);
                sendChat(&serverChat, NULL, out);
            }

            if(capCount[inPkt14.tagger_id] > team2 / 2 && capCount[inPkt14.tagger_id] > 2 && SERVER_MESSAGES){
                bzero(serverChat.message, MAX_MESSAGE);
                serverChat.sendingPlayer = MAX_PLAYERS;
                sprintf(serverChat.message, "** %s is on a Rampage!! **", pLists->playerNames[inPkt14.tagger_id]);
                sendChat(&serverChat, NULL, out);
            }

            pLists->playerTeams[inPkt14.taggee_id] = TEAM_NONE;
            pLists->readystatus[inPkt14.taggee_id] = PLAYER_STATE_OUT;

            //Check win
            countTeams(pLists->playerTeams, &team1, &team2);
            if(team2 <= 0) {
                printf("Hackers Eliminated!\n");
                gameInfo->game_status = GAME_TEAM1_WIN;
            }
            else if(team2 == 1 && SERVER_MESSAGES){
                // last man standing message
                for(i = 0; i < MAX_PLAYERS; ++i){
                    if(pLists->playerTeams[i] == TEAM_ROBBERS){
                        break;
                    }
                }

                bzero(serverChat.message, MAX_MESSAGE);
                serverChat.sendingPlayer = MAX_PLAYERS;
                sprintf(serverChat.message, "%s is the last man standing!\n", pLists->playerNames[i]);
                sendChat(&serverChat, NULL, out);
                writePacket(out, pLists, 3);
                writePacket(out, gameInfo, 8);
            }
            else{
                writePacket(out, pLists, 3);
                writePacket(out, gameInfo, 8);
            }

            break;
        default:
            DEBUG(DEBUG_ALRM, "GC> Running> Receiving invalid packet");
            clearUnexpectedPacket(in, pType);
            break;
        }
    }
}

/**
 * Handles game reset; once the game ends this sends the winner and the win message. Then sends all
 * players back to the lobby.
 *
 * Communication:
 * Sends pkt08 (PKT_GAME_STATUS) with the winner information.
 * Sends pkt03 (PKT_PLAYERS_UPDATE) with the new player infomation.
 *
 *
 * @param[in]   sockets     Array of 2 sockets for reading and writing.
 * @param[out]  pLists      Holds all information related to each player.
 * @param[out]  gameInfo    Holds all information related to game state and objectives.
 * @return void
 *
 * @designer German Villarreal
 * @author German Villarreal
 *
 * @date March 25, 2014
 */
void endController(void* sockets, PKT_PLAYERS_UPDATE *pLists, PKT_GAME_STATUS *gameInfo)
{
    SOCKET out   = ((SOCKET*) sockets)[1];     // Socket to relay network messages
    int i;
    PKT_CHAT serverChat;

    serverChat.sendingPlayer = MAX_PLAYERS;

    if(!RUNNING) {
        return;
    }

    printf("Game Over!\n");
    printf("*********************************\n");

    // Send objectives and who won
    writePacket(out, gameInfo, 8);

    // Send the player states
    for(i = 0; i < MAX_PLAYERS; ++i){
        if(pLists->playerTeams[i] != TEAM_NONE){
            pLists->readystatus[i] = PLAYER_STATE_WAITING;
            pLists->playerTeams[i] = TEAM_NONE;
        }
    }

    if(SERVER_MESSAGES){
        if(gameInfo->game_status == GAME_TEAM1_WIN){
            sprintf(serverChat.message, "Hackers Eliminated!");
            sendChat(&serverChat, NULL, out);
            bzero(serverChat.message, MAX_MESSAGE);
            sleep(2);
            sprintf(serverChat.message, "Guards Win!");
            sendChat(&serverChat, NULL, out);
            bzero(serverChat.message, MAX_MESSAGE);
            sleep(3);
        }
        else{
            sprintf(serverChat.message, "Takedown successful!");
            sendChat(&serverChat, NULL, out);
            bzero(serverChat.message, MAX_MESSAGE);
            sleep(2);
            sprintf(serverChat.message, "Hackers Win!");
            sendChat(&serverChat, NULL, out);
            bzero(serverChat.message, MAX_MESSAGE);
            sleep(3);
        }
    }

    // send new player info
    writePacket(out, pLists, 3);

    // Move them to the lobby
    forceMoveAll(sockets, pLists, PLAYER_STATE_WAITING);
}

/***********************************************************/
/******                HELPER FUNCTIONS               ******/
/***********************************************************/

/**
 * Used for moving all players to a specified floor. Players need to be moved in certain conditions such as
 * game start and game end. the status parameter is used to differentiate the positions to send to as well as
 * uses this status to set their player state.
 *
 * Communication:
 * Sends IPC_PKT_3 (PKT_FORCE_MOVE) to the GameplayController for moving them to new floor.
 *
 *
 * @param[in]   sockets     Array of 2 sockets for reading and writing.
 * @param[out]  pLists      Holds all information related to each player.
 * @param[in]   status      Used for determining the position to move to.
 * @return void
 *
 * @designer German Villarreal
 * @author German Villarreal
 *
 * @date March 24, 2014
 */
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
                floor = FLOOR_COP_START;
            }
            else if(pLists->playerTeams[i] == TEAM_ROBBERS){
                floor = FLOOR_ROBBER_START;
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
            if(pLists->readystatus[i] == PLAYER_STATE_WAITING){
                outIPC3.playerNo = i;
                outIPC3.newFloor = FLOOR_LOBBY;
                writeIPC(in, &outIPC3, IPC_PKT_3);
            }
        }
    }

}

/**
 * Used for moving all players to a specified floor. Players need to be moved in certain conditions such as
 * game start and game end. the status parameter is used to differentiate the positions to send to as well as
 * uses this status to set their player state.
 *
 * Revisions:
 *      -# March 24 - Refactored and added status parameter for multipurposing
 *
 * @param[in]   sockets     Array of 2 sockets for reading and writing.
 * @param[out]  pLists      Holds all information related to each player.
 * @param[in]   status      Used for determining the position to move to.
 * @return void
 *
 * @designer German Villarreal
 * @author German Villarreal
 *
 * @date March 14, 2014
 */
size_t countObjectives(bool_t *objectives)
{
    int i, val = 0;
    for(i = 0; i < MAX_OBJECTIVES; ++i)
    {
        if(*(objectives+i) == OBJECTIVE_CAPTURED)
            val++;
    }
    return val;
}

/**
 * Counts the number of players currently in the game. If the player has a team he is considered to be
 * active.
 *
 * Revisions:
 *      -# March 24 - Refactored and added status parameter for multipurposing
 *
 * @param[in]   playerTeams     Array of player teams
 * @return amount of players in the game
 *
 * @designer German Villarreal
 * @author German Villarreal
 *
 * @date March 25, 2014
 */
size_t countActivePlayers(const teamNo_t *playerTeams)
{
    size_t team1 = 0, team2 = 0;
    return countTeams(playerTeams, &team1, &team2);
}

/**
 * Counts the number of players currently in the game. If the player has a team he is considered to be
 * active.
 *
 * @param[in]   playerTeams   Array of player teams
 * @param[out]  team1         Number of players in TEAM_COPS
 * @param[out]  team2         Number of players in TEAM_ROBBERS
 * @return amount of players in the game
 *
 * @designer German Villarreal
 * @author German Villarreal
 *
 * @date March 12, 2014
 */
size_t countTeams(const teamNo_t *playerTeams, size_t *team1, size_t *team2)
{
    size_t i, state;
    *team1 = *team2 = 0;
    for(i = 0; i < MAX_PLAYERS; i++)
    {
        state = *(playerTeams+i);
        switch(state)
        {
            case TEAM_NONE:
                break;

            case TEAM_COPS:
                (*team1)++;
                break;

            case TEAM_ROBBERS:
                (*team2)++;
                break;

            default:
                DEBUG(DEBUG_ALRM, "GC> Error getting player's team");
                break;
        }

    }
    return (*team1) + (*team2);
}

/**
 * Counts number of players in each team and returns an array of balanced teams. Deisgned to favor TEAM_COPS
 * if uneven number of players.
 *
 * @param[in]   teamIn        Array of unbalanced player teams
 * @param[out]  teamOut       Array of balanced player teams
 * @return void
 *
 * @designer German Villarreal
 * @author German Villarreal
 *
 * @date March 12, 2014
 */
void balanceTeams(teamNo_t *teamIn, teamNo_t *teamOut)
{
    size_t team1Count = 0, team2Count = 0, i = 0;
    memcpy(teamOut, teamIn, sizeof(teamNo_t)*MAX_PLAYERS);
    countTeams(teamIn, &team1Count, &team2Count);
    bzero(teamIn, sizeof(teamNo_t) * MAX_PLAYERS);

    if(BALANCE_TEAMS){

        if(FAVOR_COPS){
            for(i = rand() % MAX_PLAYERS; team2Count < team1Count; ++i)
            {
                if(teamOut[i] == TEAM_COPS)
                {
                    teamOut[i] = TEAM_ROBBERS;
                    team2Count++;
                    team1Count--;
                }

                if(i == MAX_PLAYERS - 1){
                    i = 0;
                }
            }
        }

        // Add cops until = or greater
        for(i = rand() % MAX_PLAYERS; team1Count < team2Count; ++i)
        {
            if(teamOut[i] == TEAM_ROBBERS)
            {
                teamOut[i] = TEAM_COPS;
                team1Count++;
                team2Count--;
            }

            if(i == MAX_PLAYERS - 1){
                    i = 0;
            }
        }

        if(!FAVOR_COPS){
            for(i = rand() % MAX_PLAYERS; team2Count < team1Count; ++i)
            {
                if(teamOut[i] == TEAM_COPS)
                {
                    teamOut[i] = TEAM_ROBBERS;
                    team2Count++;
                    team1Count--;
                }

                if(i == MAX_PLAYERS - 1){
                    i = 0;
                }
            }
        }
    }
}

/**
 * Counts number of players that are ready; returns appropriate game state.
 *
 * @param[in]   teamIn        Array of unbalanced player teams
 * @param[out]  teamOut       Array of balanced player teams
 * @return GAME_STATE_ACTIVE if enough players are ready;
 *             GAME_STATE_WAITING otherwise.
 *
 * @designer German Villarreal
 * @author German Villarreal
 *
 * @date March 12, 2014
 */
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

/**
 * Resets the player lists. -useful in new game setup
 * All player data set to initial status.
 *
 * @param[out]   pLists        Array of unbalanced player teams
 * @param[in]    maxPlayers       Array of balanced player teams
 * @return GAME_STATE_ACTIVE if enough players are ready;
 *             GAME_STATE_WAITING otherwise.
 *
 * @designer German Villarreal
 * @author German Villarreal
 *
 * @date March 25, 2014
 */
void zeroPlayerLists(PKT_PLAYERS_UPDATE *pLists, const int maxPlayers)
{
    int i, j;
    for(i = 0; i < MAX_PLAYERS; ++i)
    {
        pLists->playerValid[i] = FALSE;
        pLists->characters[i] = 0;
        for(j = 0; j < MAX_NAME; ++j) {
            bzero(pLists->playerNames[i], MAX_NAME);
        }

        if(i < maxPlayers){
            pLists->readystatus[i] = PLAYER_STATE_AVAILABLE;

        } else if( i >= maxPlayers){
            pLists->readystatus[i] = PLAYER_STATE_INVALID;
        }
	}
}

/**
 * Resets the player lists. -useful in new game setup
 * All player data set to initial status.
 *
 * @param[in]    in             Socket to read from.
 * @param[out]   maxPlayers     Maximum number of players allowed.
 * @param[out]   pLists         Zeroed out player lists.

 * @return 1 on success; 0 otherwise.
 *
 * @designer German Villarreal
 * @author German Villarreal
 *
 * @date March 25, 2014
 */
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

    memcpy(serverName, pkt0.serverName, MAX_NAME);

    zeroPlayerLists(pLists, *maxPlayers);
	return 1;
}

/**
 * Resets the player lists. -useful in new game setup
 * All player data set to initial status.
 *
 * @param[in]    in             Socket to read from.
 * @param[out]   maxPlayers     Maximum number of players allowed.
 * @param[out]   pLists         Zeroed out player lists.

 * @return 1 on success; 0 otherwise.
 *
 * @designer German Villarreal
 * @author German Villarreal
 *
 * @date March 25, 2014
 */
void clearUnexpectedPacket(SOCKET sock, packet_t type){
    void* data = malloc(netPacketSizes[type]);
    getPacket(sock, data, netPacketSizes[type]);
    free(data);
}

/***********************************************************/
/******         SOCKET UTILITY FUNCTIONS              ******/
/***********************************************************/

/**
 * Sends IPC data to the IPC socket for another thread to process.
 *
 * @param[in]    sock             Socket to read from.
 * @param[in]    buf            Data to send.
 * @param[in]    type           IPC packet type to send.
 *
 * @designer German Villarreal
 * @author German Villarreal
 *
 * @date February 20, 2014
 */
inline void writeIPC(SOCKET sock, void* buf, packet_t type)
{

    write(sock, &type, sizeof(packet_t));
    type -= 0xB0;
    write(sock, buf, ipcPacketSizes[type]);

    #if DEBUG_ON
        char buff[BUFFSIZE];
        sprintf(buff, "GC> Sent packet: %d", type);
        DEBUG(DEBUG_INFO, buff);
    #endif
}

/**
 * Sends net packet to outbound network socket. Packets are sent to all clients.
 *
 * @param[in]    sock           Socket to read from.
 * @param[in]    packet         Data to send.
 * @param[in]    type           Net packet type to send.
 *
 * @designer German Villarreal
 * @author German Villarreal
 *
 * @date February 20, 2014
 */
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

/**
 * Sends net packet to outbound network socket. Packets are sent to clients if
 * specified int he mask.
 *
 * @param[in]    sock           Socket to read from.
 * @param[in]    packet         Data to send.
 * @param[in]    type           Net packet type to send.
 * @param[in]    mask           Specifies the clients to send to.
 *
 * @designer Andrew Burian
 * @author Andrew Burian
 *
 * @date February 20, 2014
 */
inline void writePacketTo(SOCKET sock, void* packet, packet_t type, OUTMASK mask){

	write(sock, &type, sizeof(packet_t));
    write(sock, packet, netPacketSizes[type]);
    write(sock, &mask, sizeof(OUTMASK));

    #if DEBUG_ON
        char buff[BUFFSIZE];
        sprintf(buff, "GC> Sent packet: %d", type);
        DEBUG(DEBUG_INFO, buff);
    #endif
}
