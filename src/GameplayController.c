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

#include "Server.h"

extern int RUNNING;

/*--------------------------------------------------------------------------------------------------------------------
 -- FUNCTION:	GameplayController
 --
 -- DATE:  February 20, 2014
 --
 -- REVISIONS: 	Chris Holisky
 --				March 11, 2014
 --				Added code to handle IPC packets 0xB1 and 0xB2
 --				Cleaned up IPC packet IPC 0xB0
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
 --	CH - February 27, 2014: Added error messages for data inconsistencies, send and receive errors
 --		Now using outbound mask
 --
 ----------------------------------------------------------------------------------------------------------------------*/
void* GameplayController(void* ipcSocks) {

	SOCKET outswitchSock;
	SOCKET gameplaySock;
	int i = 0;
	packet_t pType = -1;
	floorNo_t playerFloor = 0;
	playerNo_t thisPlayer = 0;
	packet_t outPType = 0;
	int errPacket = 0;
	int errFloor = 0;
	int errOut = 0;
	int wrongPacket = 0;

	int maxPlayers = 0;

	OUTMASK m;

	gameplaySock = ((SOCKET*) ipcSocks)[0];
	outswitchSock = ((SOCKET*) ipcSocks)[1];

	//create structs for buffers

	PKT_POS_UPDATE *bufPlayerIn = (PKT_POS_UPDATE*) malloc(sizeof(PKT_POS_UPDATE));

	//bzero(bufPlayerIn, sizeof(PKT_POS_UPDATE));

	PKT_ALL_POS_UPDATE *bufPlayerAll = malloc(sizeof(PKT_ALL_POS_UPDATE));

	//bzero(bufPlayerAll, sizeof(PKT_ALL_POS_UPDATE));

	PKT_SERVER_SETUP *bufipcPkt0 = malloc(sizeof(PKT_SERVER_SETUP));
	PKT_NEW_CLIENT *bufipcPkt1 = malloc(sizeof(PKT_NEW_CLIENT));
	PKT_LOST_CLIENT *bufipcPkt2 = malloc(sizeof(PKT_LOST_CLIENT));

	//memset(bufPkt0, 0, sizeof(PKT_SERVER_SETUP));

	//assign struct lengths before the loop so as not to keep checking
	size_t lenPktIn = sizeof(struct pkt10);
	size_t lenPktAll = sizeof(struct pkt11);

	//Create array of floor structures
	PKT_ALL_POS_UPDATE floorArray[MAX_FLOORS + 1];
	for (i = 0; i <= MAX_FLOORS; i++) {
		floorArray[i].floor = i;
	}

	getPacketType(gameplaySock);
	getPacket(gameplaySock, bufipcPkt0, ipcPacketSizes[0]);
	maxPlayers = bufipcPkt0->maxPlayers + 1;
	DEBUG("GP> Setup Complete");

	while (RUNNING) {

		//get packet type.  Save to int.
		pType = getPacketType(gameplaySock);

		//switch to handle each type of packet
		switch (pType) {
		case -1:
			break;
		case 0xB1:
			bzero(bufipcPkt1, sizeof(ipcPacketSizes[1]));
			if (getPacket(gameplaySock, bufipcPkt1, ipcPacketSizes[1]) == -1) {
				//couldn't read packet
				errPacket++;
				fprintf(stderr, "Gameplay Controller - error reading packet 0xB1.  Count:%d\n", errPacket);

				break;
			}
			floorArray[0].players_on_floor[bufPlayerIn->player_number] = 1;
			break;
		case 0xB2:
			bzero(bufipcPkt2, sizeof(ipcPacketSizes[2]));
			if (getPacket(gameplaySock, bufipcPkt2, ipcPacketSizes[2]) == -1) {
				//couldn't read packet
				errPacket++;
				fprintf(stderr, "Gameplay Controller - error reading packet 0xB2.  Count:%d\n", errPacket);
				break;
			}
			for (i = 0; i < MAX_FLOORS + 1; i++) {
				floorArray[i].players_on_floor[bufipcPkt2->playerNo] = 0;
			}
			break;
		case 1:
			break;
		case 10:
			bzero(bufPlayerIn, sizeof(*bufPlayerIn));
			if (getPacket(gameplaySock, bufPlayerIn, lenPktIn) == -1) {
				//couldn't read packet
				errPacket++;
				fprintf(stderr, "Gameplay Controller - error reading packet 10.  Count:%d\n", errPacket);

				/*
				 *  handle error here.  Perhaps check the size of the packet as well?
				 */
				break;
			}
			//get floor number from incoming packet
			playerFloor = (bufPlayerIn->floor);

			//check to see if the floor number falls in the valid range
			if (playerFloor < 0 || playerFloor > MAX_FLOORS) {
				//break;
			}

			//get player number from incoming packet
			thisPlayer = (*bufPlayerIn).player_number;

			//check to see if the player number falls in the valid range
			if (thisPlayer < 0 || thisPlayer > maxPlayers) {
				//break;
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
				errFloor++;
				fprintf(stderr, "Gameplay Controller - player floor incorrect.  Count:%d\n", errFloor);

				//for now, we just assign the player to this floor
				floorArray[playerFloor].players_on_floor[thisPlayer] = 1;
			}

			//put position and velocity in update packet
			floorArray[playerFloor].xPos[thisPlayer] = bufPlayerIn->xPos;
			floorArray[playerFloor].yPos[thisPlayer] = bufPlayerIn->yPos;
			floorArray[playerFloor].xVel[thisPlayer] = bufPlayerIn->xVel;
			floorArray[playerFloor].yVel[thisPlayer] = bufPlayerIn->yVel;

			//stamp the time on the packet to make Shane happy  :p
			//floorArray[playerFloor].timestamp = clock();

			//set packet type for outbound server
			outPType = 11;

			//set outbound mask for outbound server
			OUT_ZERO(m);
			for (i = 0; i < MAX_PLAYERS; i++) {
				if (floorArray[playerFloor].players_on_floor[i] == 1) {
					OUT_SET(m, i);
				}
			}
			//send packet type and then packet to outbound switchboard
			/*
			 * Remove this from the switch, perhaps put it into an if block
			 * with a goodToSend boolean or something
			 */
			if (write(outswitchSock, &outPType, sizeof(outPType)) == -1) {
				errOut++;
				fprintf(stderr, "Gameplay Controller - sending to outbound switchboard.  Count:%d\n", errOut);
			}
			if (write(outswitchSock, &floorArray[playerFloor], lenPktAll) == -1) {
				errOut++;
				fprintf(stderr, "Gameplay Controller - sending to outbound switchboard.  Count:%d\n", errOut);
			}
			if (write(outswitchSock, &m, 1) == -1) {
				errOut++;
				fprintf(stderr, "Gameplay Controller - sending to outbound switchboard.  Count:%d\n", errOut);
			}

			break;

		default:
			wrongPacket++;
			fprintf(stderr, "Gameplay Controller - received unknown packet type:%d  Count:%d\n", pType, wrongPacket);

			break;
		}

		pType = -1;
	}
	free(bufPlayerAll);
	free(bufPlayerIn);
	return 0;
}
