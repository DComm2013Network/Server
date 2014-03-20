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

void writeType2(SOCKET sock, void* packet, packet_t type, OUTMASK m);

/*--------------------------------------------------------------------------------------------------------------------
 -- FUNCTION:	GameplayController
 --
 -- DATE:  February 20, 2014
 --
 -- REVISIONS: 	Chris Holisky
 --				March 11, 2014
 --				Added code to handle IPC packets 0xB1 and 0xB2
 --				Cleaned up IPC packet IPC 0xB0
 --				Initialized floor array to 0
 --				Added handling for packet 12 (floor change)
 --				Added packet 11 and 13 echo in packet 0xB1
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
	int j = 0;
	packet_t pType = -1;
	floorNo_t playerFloor = 0;
	floorNo_t newFloor = 0;
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
	PKT_ALL_POS_UPDATE *bufPlayerAll = malloc(sizeof(PKT_ALL_POS_UPDATE));
	PKT_FLOOR_MOVE_REQUEST *bufFloorMoveReq = malloc(sizeof(PKT_FLOOR_MOVE_REQUEST));
	PKT_FLOOR_MOVE *bufFloorMove = malloc(sizeof(PKT_FLOOR_MOVE));

	PKT_SERVER_SETUP *bufipcPkt0 = malloc(sizeof(PKT_SERVER_SETUP));
	PKT_NEW_CLIENT *bufipcPkt1 = malloc(sizeof(PKT_NEW_CLIENT));
	PKT_LOST_CLIENT *bufipcPkt2 = malloc(sizeof(PKT_LOST_CLIENT));
	PKT_FORCE_MOVE *bufipcPkt3 = malloc(sizeof(PKT_FORCE_MOVE));

	//bzero(bufPlayerIn, sizeof(PKT_POS_UPDATE));
	//bzero(bufPlayerAll, sizeof(PKT_ALL_POS_UPDATE));
	//memset(bufPkt0, 0, sizeof(PKT_SERVER_SETUP));

	//assign struct lengths before the loop so as not to keep checking
	size_t lenPktIn = sizeof(struct pkt10);
	size_t lenPktAll = sizeof(struct pkt11);
	size_t lenPktFloorReq = sizeof(struct pkt12);
	size_t lenPktFloor = sizeof(struct pkt13);


	//Create array of floor structures
	PKT_ALL_POS_UPDATE floorArray[MAX_FLOORS + 1];
	bzero(floorArray, sizeof(PKT_ALL_POS_UPDATE) * (MAX_FLOORS + 1));
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

		case 0xB1: //new player packet
			DEBUG("GP> Receive packet 0xB1");

			bzero(bufipcPkt1, sizeof(ipcPacketSizes[1]));
			if (getPacket(gameplaySock, bufipcPkt1, ipcPacketSizes[1]) == -1) {
				//couldn't read packet
				errPacket++;
				fprintf(stderr, "Gameplay Controller - error reading packet 0xB1.  Count:%d\n", errPacket);

				break;
			}

			//get player information, set to position 50,50 on floor 0
			bzero(bufFloorMove, netPacketSizes[13]);
			thisPlayer = bufipcPkt1->playerNo;
			playerFloor = 0;
			bufFloorMove->new_floor = playerFloor;
			bufFloorMove->xPos = getLobbyX(bufipcPkt1->playerNo);
			bufFloorMove->yPos = getLobbyY(bufipcPkt1->playerNo);

			//add player to floor array
			floorArray[0].players_on_floor[thisPlayer] = 1;

			//send floor change packet
			DEBUG("GP> Sending packet 13")
			;
			//set outbound mask for outbound server
			OUT_ZERO(m);
			OUT_SET(m, thisPlayer);
			outPType = 13;

			if (write(outswitchSock, &outPType, sizeof(outPType)) == -1) {
				errOut++;
				fprintf(stderr, "Gameplay Controller - sending to outbound switchboard.  Count:%d\n", errOut);
			}
			if (write(outswitchSock, bufFloorMove, lenPktFloor) == -1) {
				errOut++;
				fprintf(stderr, "Gameplay Controller - sending to outbound switchboard.  Count:%d\n", errOut);
			}
			if (write(outswitchSock, &m, sizeof(OUTMASK)) == -1) {
				errOut++;
				fprintf(stderr, "Gameplay Controller - sending to outbound switchboard.  Count:%d\n", errOut);
			}

			//send packet 11 to players on floor
			DEBUG("GP> Sending packet 11 to floor 0");

			outPType = 11;
			floorArray[playerFloor].xPos[thisPlayer] = bufFloorMove->xPos;
			floorArray[playerFloor].yPos[thisPlayer] = bufFloorMove->yPos;
			floorArray[playerFloor].xVel[thisPlayer] = 0;
			floorArray[playerFloor].yVel[thisPlayer] = 0;

			//set outbound mask for outbound server
			OUT_ZERO(m);
			for (i = 0; i < MAX_PLAYERS; i++) {
				if (floorArray[playerFloor].players_on_floor[i] == 1) {
					OUT_SET(m, i);
				}
			}

			if (write(outswitchSock, &outPType, sizeof(outPType)) == -1) {
				errOut++;
				fprintf(stderr, "Gameplay Controller - sending to outbound switchboard.  Count:%d\n", errOut);
			}
			if (write(outswitchSock, &floorArray, lenPktAll) == -1) {
				errOut++;
				fprintf(stderr, "Gameplay Controller - sending to outbound switchboard.  Count:%d\n", errOut);
			}
			if (write(outswitchSock, &m, sizeof(OUTMASK)) == -1) {
				errOut++;
				fprintf(stderr, "Gameplay Controller - sending to outbound switchboard.  Count:%d\n", errOut);
			}

			break;

		case 0xB2: //lost or quit client

			DEBUG("GP> Received packet 0xB2")
			;

			bzero(bufipcPkt2, ipcPacketSizes[2]);
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

        case 0xB3:  // Force floor change

            DEBUG("GP> Received packet 0xB3");

            bzero(bufipcPkt3, ipcPacketSizes[3]);

            if (getPacket(gameplaySock, bufipcPkt3, ipcPacketSizes[3]) == -1) {
				//couldn't read packet
				errPacket++;
				fprintf(stderr, "Gameplay Controller - error reading packet 0xB3.  Count:%d\n", errPacket);
				break;
			}

			// Find the player's floor
			for(i = 0; i < MAX_FLOORS; ++i){
                if(floorArray[i].players_on_floor[bufipcPkt3->playerNo]){
                    // found player
                    break;
                }
			}

			if(i == bufipcPkt3->newFloor){
                // player is already on target floor
                // do nothing
                break;
			}

			// remove player from floor
			floorArray[i].players_on_floor[bufipcPkt3->playerNo] = 0;

			// add them to the new floor
			floorArray[bufipcPkt3->newFloor].players_on_floor[bufipcPkt3->playerNo] = 1;

            // prepare outbound packet
            bufFloorMove->new_floor = bufipcPkt3->newFloor;

            // put them in their lobby location
            bufFloorMove->xPos = getLobbyX(bufipcPkt3->playerNo);
            bufFloorMove->yPos = getLobbyY(bufipcPkt3->playerNo);

            // send the player floor move
            outPType = 13;
            OUT_ZERO(m);
            OUT_SET(m, bufipcPkt3->playerNo);
            write(outswitchSock, &outPType, sizeof(packet_t));
            write(outswitchSock, bufFloorMove, netPacketSizes[13]);
            write(outswitchSock, &m, sizeof(OUTMASK));
            DEBUG("CP> Sending packet 13");


            //send updated data to all players on old floor
			outPType = 11;

			//set outbound mask for outbound server
			OUT_ZERO(m);
			for (j = 0; j < MAX_PLAYERS; j++) {
				if (floorArray[i].players_on_floor[j] == 1) {
					OUT_SET(m, j);
				}
			}
			write(outswitchSock, &outPType, sizeof(packet_t));
			write(outswitchSock, &floorArray[i], netPacketSizes[11]);
			write(outswitchSock, &m, sizeof(OUTMASK));
			DEBUG("CP> Sending packet 11");

			//send updated data to all players on new floor
			//set outbound mask for outbound server
			OUT_ZERO(m);
			for (j = 0; j < MAX_PLAYERS; j++) {
				if (floorArray[bufipcPkt3->newFloor].players_on_floor[j] == 1) {
					OUT_SET(m, j);
				}
			}
			write(outswitchSock, &outPType, sizeof(packet_t));
			write(outswitchSock, &floorArray[bufipcPkt3->newFloor], netPacketSizes[11]);
			write(outswitchSock, &m, sizeof(OUTMASK));
			DEBUG("CP> Sending packet 11");

            break;

		case 10: //player position update
			DEBUG("GP> Received packet 10")
			;

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
				//floorArray[playerFloor].players_on_floor[thisPlayer] = 1;
			}

			//put position and velocity in update packet
			floorArray[playerFloor].xPos[thisPlayer] = bufPlayerIn->xPos;
			floorArray[playerFloor].yPos[thisPlayer] = bufPlayerIn->yPos;
			floorArray[playerFloor].xVel[thisPlayer] = bufPlayerIn->xVel;
			floorArray[playerFloor].yVel[thisPlayer] = bufPlayerIn->yVel;

			DEBUG("GP> Sending packet 11 to all players on floor")
						;

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
			if (write(outswitchSock, &outPType, sizeof(outPType)) == -1) {
				errOut++;
				fprintf(stderr, "Gameplay Controller - sending to outbound switchboard.  Count:%d\n", errOut);
			}
			if (write(outswitchSock, &floorArray[playerFloor], lenPktAll) == -1) {
				errOut++;
				fprintf(stderr, "Gameplay Controller - sending to outbound switchboard.  Count:%d\n", errOut);
			}
			if (write(outswitchSock, &m, sizeof(OUTMASK)) == -1) {
				errOut++;
				fprintf(stderr, "Gameplay Controller - sending to outbound switchboard.  Count:%d\n", errOut);
			}

			break;
		case 12: //floor change
			DEBUG("GP> Received packet 12")
			;

			bzero(bufFloorMoveReq, sizeof(*bufFloorMoveReq));
			if (getPacket(gameplaySock, bufFloorMoveReq, lenPktFloorReq) == -1) {
				//couldn't read packet
				errPacket++;
				fprintf(stderr, "Gameplay Controller - error reading packet 12.  Count:%d\n", errPacket);

				/*
				 *  handle error here.  Perhaps check the size of the packet as well?
				 */
				break;
			}

			//get information from floor move request
			playerFloor = bufFloorMoveReq->current_floor;
			newFloor = bufFloorMoveReq->desired_floor;
			thisPlayer = bufFloorMoveReq->player_number;

			//set information for floor move
			bzero(bufFloorMove, sizeof(*bufFloorMove));
			bufFloorMove->new_floor = newFloor;
			bufFloorMove->xPos = bufFloorMoveReq->desired_xPos;
			bufFloorMove->yPos = bufFloorMoveReq->desired_yPos;

			DEBUG("GP> Sending packet 13")
						;

			//set outbound mask to send only to this player
			OUT_ZERO(m);
			OUT_SET(m, thisPlayer);

			outPType = 13;
			if (write(outswitchSock, &outPType, sizeof(outPType)) == -1) {
				errOut++;
				fprintf(stderr, "Gameplay Controller - sending to outbound switchboard.  Count:%d\n", errOut);
			}
			if (write(outswitchSock, bufFloorMove, lenPktFloor) == -1) {
				errOut++;
				fprintf(stderr, "Gameplay Controller - sending to outbound switchboard.  Count:%d\n", errOut);
			}
			if (write(outswitchSock, &m, sizeof(OUTMASK)) == -1) {
				errOut++;
				fprintf(stderr, "Gameplay Controller - sending to outbound switchboard.  Count:%d\n", errOut);
			}

			DEBUG("GP> Sending packet 11 to old floor")
						;

			//send updated data to all players on old floor
			outPType = 11;

			//remove this player from the floor
			floorArray[playerFloor].players_on_floor[thisPlayer] = 0;

			//set outbound mask for outbound server
			OUT_ZERO(m);
			for (i = 0; i < MAX_PLAYERS; i++) {
				if (floorArray[playerFloor].players_on_floor[i] == 1) {
					OUT_SET(m, i);
				}
			}
			if (write(outswitchSock, &outPType, sizeof(outPType)) == -1) {
				errOut++;
				fprintf(stderr, "Gameplay Controller - sending to outbound switchboard.  Count:%d\n", errOut);
			}
			if (write(outswitchSock, &floorArray[playerFloor], lenPktAll) == -1) {
				errOut++;
				fprintf(stderr, "Gameplay Controller - sending to outbound switchboard.  Count:%d\n", errOut);
			}
			if (write(outswitchSock, &m, sizeof(OUTMASK)) == -1) {
				errOut++;
				fprintf(stderr, "Gameplay Controller - sending to outbound switchboard.  Count:%d\n", errOut);
			}

			//Send updated player position to all players on the new floor

			//put position and velocity in update packet
			floorArray[newFloor].xPos[thisPlayer] = bufFloorMove->xPos;
			floorArray[newFloor].yPos[thisPlayer] = bufFloorMove->yPos;
			floorArray[newFloor].xVel[thisPlayer] = 0;
			floorArray[newFloor].yVel[thisPlayer] = 0;

			//add this player to the new floor
			floorArray[newFloor].players_on_floor[thisPlayer] = 1;

			//set outbound mask for outbound server
			OUT_ZERO(m);
			for (i = 0; i < MAX_PLAYERS; i++) {
				if (floorArray[newFloor].players_on_floor[i] == 1) {
					OUT_SET(m, i);
				}
			}
			DEBUG("GP> Sending packet 11 to new floor")
			;
			//writeType(outswitchSock, &bufFloorMove,outPType, m);

			if (write(outswitchSock, &outPType, sizeof(outPType)) == -1) {
				errOut++;
				fprintf(stderr, "Gameplay Controller - sending to outbound switchboard.  Count:%d\n", errOut);
			}
			if (write(outswitchSock, &floorArray[newFloor], lenPktAll) == -1) {
				errOut++;
				fprintf(stderr, "Gameplay Controller - sending to outbound switchboard.  Count:%d\n", errOut);
			}
			if (write(outswitchSock, &m, sizeof(OUTMASK)) == -1) {
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

void writeType2(SOCKET sock, void* packet, packet_t type, OUTMASK m) {
	write(sock, &type, sizeof(packet_t));
	if(type >= 0xB0){
		write(sock, packet, sizeof(packet));
	}
	else{
		write(sock, packet, netPacketSizes[type]);
	}
	write(sock, &m, sizeof(OUTMASK));
}
