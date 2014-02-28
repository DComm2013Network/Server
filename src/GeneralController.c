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

/*--------------------------------------------------------------------------------------------------------------------
 -- FUNCTION:	GeneralController
 --
 -- DATE: 		20 February 2014
 --
 -- REVISIONS: 	none
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
	//SOCKET generalSock;
	//SOCKET outswitchSock;

	bool objCaptured[MAX_OBJECTIVES];
	status_t status = GAME_STATE_WAITING;
	size_t numPlayers = 0;
	packet_t inPktType, outPktType = 8;
	int i, maxPlayers;
	//TODO: store players into teams

	OUTMASK m;


	/* Will look into changing this... */
	PKT_SERVER_SETUP *pkt0;
	PKT_NEW_CLIENT *pkt1;
	PKT_LOST_CLIENT *pkt2;
	//PKT_SERVER_SETUP *pktServerSetup;
	PKT_GAME_STATUS *pktGameStatus;

	//size_t pkt0Size, pkt1Size, pkt2Size, pktServerSetupSize, pktGameStatusSize;
	//pkt0Size = sizeof(IPC_PKT_0);
	//pkt1Size = sizeof(IPC_PKT_1);
	//pkt2Size = sizeof(IPC_PKT_2);
	//pktServerSetupSize = sizeof(PKT_SERVER_SETUP);
	//pktGameStatusSize = sizeof(PKT_GAME_STATUS);

	pkt0 = malloc(ipcPacketSizes[0]);
	pkt1 = malloc(ipcPacketSizes[1]);
	pkt2 = malloc(ipcPacketSizes[2]);
	//pktServerSetup = malloc(netPacketSizes[5]);
	pktGameStatus = malloc(netPacketSizes[8]);

	SOCKET generalSock   = ((SOCKET*) ipcSocks)[0];
	SOCKET outswitchSock = ((SOCKET*) ipcSocks)[1];


	// <--------------------------------------------------------------->			

	// wait ipc 0	
	if ((inPktType = getPacketType(generalSock)) != IPC_PKT_0) {
		fprintf(stderr, "Expected packet type: %d\nReceived: %d\n", IPC_PKT_0, inPktType);
		return NULL;
	}
	getPacket(generalSock, pkt0, ipcPacketSizes[0]);
	//game initialized
	
	maxPlayers = pkt0->maxPlayers;

	while (RUNNING) {
		inPktType = getPacketType(generalSock);
		switch (inPktType) {
		case IPC_PKT_1: // New Player
			if (numPlayers == maxPlayers)
				break;

			getPacket(generalSock, pkt1, ipcPacketSizes[2]);
			numPlayers++;
			// TODO: add to team

			if (numPlayers == MAX_PLAYERS)
				status = GAME_STATE_ACTIVE;
			break;

		case IPC_PKT_2: // Player lost
			if (numPlayers < 1)
				break;

			getPacket(generalSock, pkt2, ipcPacketSizes[2]);
			numPlayers--;

			//TODO: in comments
			if (numPlayers < 2) // or last player from a team dropped
					{
				// get dropped player/active player team
				// set game winner
				status = GAME_STATE_OVER;
			}
			break;

		case 8: // Game Status
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
			break;
		default:
			DEBUG("GC> Receiving packets it shouldn't")
			;
			break;
		}

		memcpy(pktGameStatus->objectives_captured, objCaptured, MAX_OBJECTIVES);
		pktGameStatus->game_status = status;

		OUT_ZERO(m);
		OUT_SETALL(m);

		write(outswitchSock, &outPktType, 1);
		write(outswitchSock, pktGameStatus, netPacketSizes[8]);
		write(outswitchSock, &m, 1);
	}
	return NULL;
}
