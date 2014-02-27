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

void* GeneralController(void* ipcSocks){
	
	bool objCaptured[MAX_OBJECTIVES];
	status_t status = GAME_STATE_WAITING;
	size_t 	numPlayers = 0;
	int inPktType, i;
	//TODO: store players into teams
	
	SOCKET generalSock = ((SOCKET*)ipcSocks)[0];
	SOCKET outswitchSock = ((SOCKET*)ipcSocks)[1];
	
	/* Will look into changing this... */
	PKT_SERVER_SETUP	*pkt0;
	PKT_NEW_CLIENT		*pkt1;
	PKT_LOST_CLIENT		*pkt2;
	PKT_SERVER_SETUP	*pktServerSetup;
	PKT_GAME_STATUS		*pktGameStatus;
		
	
	size_t pkt0Size, pkt1Size, pkt2Size, pktServerSetupSize, pktGameStatusSize;
	pkt0Size 			= sizeof(IPC_PKT_0		  );
	pkt1Size 			= sizeof(IPC_PKT_1		  );
	pkt2Size 			= sizeof(IPC_PKT_2		  );
	pktServerSetupSize 	= sizeof(PKT_SERVER_SETUP );
	pktGameStatusSize 	= sizeof(PKT_GAME_STATUS  );
	
	pkt0 			= malloc(pkt0Size	 		);
	pkt1 			= malloc(pkt1Size	 		);
	pkt2 			= malloc(pkt2Size	 		);
	pktServerSetup	= malloc(pktServerSetupSize	);
	pktGameStatus	= malloc(pktGameStatusSize 	);
	
	memset(objCaptured, 	0, MAX_OBJECTIVES	  );
	memset(pkt0, 			0, pkt0Size 		  );
	memset(pkt1, 			0, pkt1Size 		  );
	memset(pkt2, 			0, pkt2Size 		  );
	memset(pktServerSetup, 	0, pktServerSetupSize );
	memset(pktGameStatus, 	0, pktGameStatusSize  );
	// <--------------------------------------------------------------->			
	
	// wait ipc 0	
	if((inPktType = getPacketType(generalSock)) != IPC_PKT_0)
	{
		fprintf(stderr, "Expected packet type: %d\nReceived: %d\n", IPC_PKT_0, inPktType);
		return NULL;
	}
	getPacket(generalSock, pkt0, pkt0Size);
	//game initialized
	
    while(RUNNING)
    {
        inPktType = getPacketType(generalSock);
		switch(inPktType)	
		{
			case IPC_PKT_1: // New Player
				if(numPlayers == MAX_PLAYERS)
					break;

				getPacket(generalSock, pkt1, pkt1Size);
				numPlayers++;
				// TODO: add to team
				
				if(numPlayers == MAX_PLAYERS)
					status = GAME_STATE_ACTIVE;
			break;
			
			case IPC_PKT_2: // Player lost		
				if(numPlayers < 1)
					break;
				
				getPacket(generalSock, pkt2, pkt2Size);
				numPlayers--;
				
				//TODO: in comments
				if(numPlayers < 2) // or last player from a team dropped
				{
					// get dropped player/active player team
					// set game winner
					status  = GAME_STATE_OVER;
				}
			break;
					
			case 8: // Game Status
				getPacket(generalSock, pktGameStatus, pktGameStatusSize);
				if(status == GAME_STATE_ACTIVE)
				{
					//update objective listing
					int countCaptured = 0;
					// Copy the client's objective listing to the server.
					// May need some better handling; if 2 are received very close to each other
					 memcpy(objCaptured, pktGameStatus->objectives_captured, MAX_OBJECTIVES);

					//check win
					for(i = 0; i < MAX_OBJECTIVES; i++)
						if(objCaptured[i] == 1)
							countCaptured++;

					if(countCaptured >= winRatio)
						status = GAME_STATE_OVER;
				}
			break;
		}
		
		
		int outPktType = 8;
		memcpy(pktGameStatus->objectives_captured, objCaptured, MAX_OBJECTIVES);
		pktGameStatus->game_status = status;

		OUTMASK m; OUT_ZERO(m); OUT_SETALL(m);
		
		write(outswitchSock, &outPktType, 1);
		write(outswitchSock, pktGameStatus, pktGameStatusSize);
		write(outswitchSock, &m, 1);
	}
	return NULL;
}
