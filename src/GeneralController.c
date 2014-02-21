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


#include "NetComm.h"
#include "Server.h"

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

/*
 typedef struct pkt01{
	char 		client_player_name[MAX_NAME];
} PKT_PLAYER_JOIN;

typedef struct pkt02{
	unsigned int 		connect_code;
	playerNo_t 	clients_player_number;
	teamNo_t 	clients_team_number;
} PKT_JOIN_RESPONSE;

typedef struct pkt08{
	bool		objectives_captured[MAX_OBJECTIVES];
	status_t	game_status;
} PKT_GAME_STATUS;

*/
int GeneralController(SOCKET generalSock, SOCKET outswitchSock) {
	
	bool objectivesCaptured[MAX_OBJECTIVES];
	playerNo_t playerList[MAX_PLAYERS];	
	
	
	// initialize objectives array
    PKT_READY_STATUS pkt;
    size_t pktSize = sizeof(pkt);
    
    //while 1
    while(RUNNING)
    {
        // listen on ipc socket
        read(gneralSock, &pkt, pktSize);
        // how do i check what packet type i read
        
        // if read pkt 01
			//insert player name to ___?
			//insert into playerlist
			//determine teamnumber 
			//	= { loop through playerlist
			//		count teams }
			// send pkt5
			
        // if read pkt 02
			//___?
			
        // if read pkt 08
			//merge pkt8.objectives captured
				//with local
			//send pkt08 with merged objectives
        
	}
	return -99;
}

/*
typedef struct pkt05{
	playerNo_t	player_number;
	status_t	ready_status;
	teamNo_t	team_number;
	char 		player_name[MAX_NAME];
	clock_t		timestamp;
} PKT_READY_STATUS;

typedef struct pkt08{
	bool		objectives_captured[MAX_OBJECTIVES];
	status_t	game_status;
} PKT_GAME_STATUS;
*/
