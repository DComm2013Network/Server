/*-------------------------------------------------------------------------------------------------------------------*
 -- SOURCE FILE: .c
 --		The Process will receive individual player updates, add them to the complete list
 --			and then send that updated package to outbound switchboard
 --
 -- FUNCTIONS:
 -- 		int GameplayController(SOCKET gameplaySock, SOCKET outswitchSock)
 --
 --
 -- DATE:  February 20, 2014
 --
 -- REVISIONS: 	none
 --
 -- DESIGNER: 	Andrew Burian
 --
 -- PROGRAMMER: Chris Holisky
 --
 -- NOTES:
 --
 *-------------------------------------------------------------------------------------------------------------------*/

#include "NetComm.h"
#include "Server.h"

/*--------------------------------------------------------------------------------------------------------------------
 -- FUNCTION:	GameplayController
 --
 -- DATE:  February 20, 2014
 --
 -- REVISIONS: 	none
 --
 -- DESIGNER:	Andrew Burian / Chris Holisky
 --
 -- PROGRAMMER:	Chris Holisky
 --
 -- INTERFACE: 	int GameplayController(SOCKET gameplaySock, SOCKET outswitchSock)
 --					SOCKET gameplaySock: socket that leads to this function
 --					SOCKET outswitchSock: socket that leads to the outbound switchboard
 --
 -- RETURNS: 	int
 --					failure:	-99 Not yet implemented
 --								-1  General failure / error number not assigned
 --					success: 	0
 --
 -- NOTES:  Each floor will have it's own controller.  This creates a master list for the floor, gets updates
 --		from the inbound switchboard, adds these updates to the master file and sends that to the outbound
 --		switchboard for distribution to players on that floor.
 --		Takes packets 0, 1, 2, 10.
 --		Sends packets 11
 --	CH - February 20, 2014: There is currently no error checking, handling or correction in place.  This will come
 --		in later updates.
 --		For Milestone 1 only a small part of the functionality is in place.
 --		Takes packets 10
 --		Sends packets 11
 --
 ----------------------------------------------------------------------------------------------------------------------*/
int GameplayController(SOCKET gameplaySock, SOCKET outswitchSock) {
	/*
	 * GAMEPLAY CONTROLLER
	 *just milestone1*
	 while 1
	 PKT_POS_UPDATE
	 listen on ipc-socket for a packet 10
	 read player information
	 update master player information
	 send to outbound switchboard
	 */


	//create structs for buffers
	PKT_POS_UPDATE *bufPlayerIn = malloc(sizeof(PKT_POS_UPDATE));
	memset(&bufPlayerIn, 0, sizeof(PKT_POS_UPDATE));

	//This will end up being a master file for this floor controller
	PKT_ALL_POS_UPDATE *bufPlayerAll = malloc(sizeof(PKT_ALL_POS_UPDATE));
	memset(&bufPlayerAll, 0, sizeof(PKT_ALL_POS_UPDATE));

	//assign struct lengths before the loop so as not to keep checking
	size_t lenPktIn = sizeof(struct pkt10);
	size_t lenPktAll = sizeof(struct pkt11);

	while (1) {
		//get the packet from the inbound packet
		read(gameplaySock, bufPlayerIn, lenPktIn);

		//get player number
		playerNo_t thisPlayer = bufPlayerIn->player_number;

		//assign player's floor to identifier for all players packet
		bufPlayerAll->floor = bufPlayerIn->floor;

		//register this player on floor
		bufPlayerAll->players_on_floor[thisPlayer]=1;

		//put position and velocity in update packet
		bufPlayerAll->xPos[thisPlayer] = bufPlayerIn->xPos;
		bufPlayerAll->yPos[thisPlayer] = bufPlayerIn->yPos;
		bufPlayerAll->xVel[thisPlayer] = bufPlayerIn->xVel;
		bufPlayerAll->yVel[thisPlayer] = bufPlayerIn->yVel;

		//stamp the time on the packet to make Shane happy  :p
		bufPlayerAll->timestamp = bufPlayerIn->timestamp;

		//write the packet to th eoutbound server
		write(outswitchSock, bufPlayerAll, lenPktAll);
	}
	return 0;
}
