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
#include "Sockets.h"

extern int RUNNING;

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
 --		Takes packets:
 --			IPC_PKT_0 0xB0	PKT_SERVER_SETUP	pktB0 - uses maxplayers
 --			IPC_PKT_1 0xB1	PKT_NEW_CLIENT		pktB1 - uses playerNo
 --							PKT_POS_UPDATE		pkt10 - uses whole packet
 --		Sends packets:
 --							PKT_ALL_POS_UPDATE	pkt11 - uses whole packet

 --	CH - February 20, 2014: There is currently no error checking, handling or correction in place.  This will come
 --		in later updates.
 --		For Milestone 1 only a small part of the functionality is in place.
 --		Takes packets 10
 --		Sends packets 11
 --
 ----------------------------------------------------------------------------------------------------------------------*/
int GameplayController(SOCKET gameplaySock, SOCKET outswitchSock) {

	int i = 0;
	int pType = -1;
	int playerFloor = -1;
	playerNo_t thisPlayer = -1;
	int outPType = -1;

	//create structs for buffers

	/*
	 * set up other packets:
	 * 	incoming IPC_PKT_0 0xB0
	 * 	incoming IPC_PKT_1 0xB1
	 */

	PKT_POS_UPDATE *bufPlayerIn = malloc(sizeof(PKT_POS_UPDATE));
	memset(&bufPlayerIn, 0, sizeof(PKT_POS_UPDATE));

	PKT_ALL_POS_UPDATE *bufPlayerAll = malloc(sizeof(PKT_ALL_POS_UPDATE));
	memset(&bufPlayerAll, 0, sizeof(PKT_ALL_POS_UPDATE));

	//assign struct lengths before the loop so as not to keep checking

	/*
	 * set up other struct lengths:
	 * 	incoming IPC_PKT_0 0xB0
	 * 	incoming IPC_PKT_1 0xB1
	 */

	size_t lenPktIn = sizeof(struct pkt10);
	size_t lenPktAll = sizeof(struct pkt11);

	//Create array of floor structures
	PKT_ALL_POS_UPDATE floorArray[MAX_FLOORS + 1];
	for (i = 0; i <= MAX_FLOORS; i++) {
		floorArray[i].floor = i;
	}

	while (RUNNING) {

		//get packet type.  Save to int.
		pType = getPacketType(gameplaySock);

		//switch to handle each type of packet
		switch (pType) {
		case 0:
			break;
		case 1:
			break;
		case 10:

			if (getPacket(gameplaySock, bufPlayerIn, lenPktIn) == -1) {
				//couldn't read packet
				/*
				 *  handle error here.  Perhaps check the size of the packet as well?
				 */
				break;
			}
			//get floor number from incoming packet

			playerFloor = bufPlayerIn->floor;

			//check to see if the floor number falls in the valid range
			if (playerFloor < 0 || playerFloor > 8) {
				break;
			}

			//get player number from incoming packet
			thisPlayer = bufPlayerIn->player_number;

			//check to see if the player number falls in the valid range
			if (thisPlayer < 0 || thisPlayer > 31) {
				break;
			}

			//is this player supposed to be on this floor?
			if (floorArray[playerFloor].players_on_floor[thisPlayer] != 1) {
				/*
				 * handle error - this means that the player is sending a
				 * packet with the wrong floor number.  Perhaps pop up a big
				 * message calling them a dirty cheater or just ignore this
				 * packet as corrupt.  If we ignore, we will probably want to
				 * put in a count so that if several bad floor packets come in
				 * from this player we can figure out if we didn't get their
				 * floor update.
				 */
				//for now, we just assign the player to this floor
				floorArray[playerFloor].players_on_floor[thisPlayer] = 1;
			}

			//put position and velocity in update packet
			floorArray[playerFloor].xPos[thisPlayer] = bufPlayerIn->xPos;
			floorArray[playerFloor].yPos[thisPlayer] = bufPlayerIn->yPos;
			floorArray[playerFloor].xVel[thisPlayer] = bufPlayerIn->xVel;
			floorArray[playerFloor].yVel[thisPlayer] = bufPlayerIn->yVel;

			//stamp the time on the packet to make Shane happy  :p
			floorArray[playerFloor].timestamp = clock();

			//write the packet to the outbound server
			outPType = 11;

			//send packet type and then packet to outbound switchboard
			/*
			 * Remove this from the switch, perhaps put it into an if block
			 * with a goodToSend boolean or something
			 */
			write(outswitchSock, &outPType, sizeof(outPType));
			write(outswitchSock, &floorArray[playerFloor], lenPktAll);

			break;

		default:
			break;
		}
	}

	return 0;
}
