/** @ingroup Server */
/** @{ */

/**
 * The Process will receive individual player updates, add them to the complete list
 * and then send that updated package to outbound switchboard
 *
 * @file GameplayController.c
 */

/** @} */

#include "Server.h"

extern int RUNNING;

//void writeType2(SOCKET sock, void* packet, packet_t type, OUTMASK m);


 /**
 * This creates a master list for the floor, gets updates
 *		from the inbound switchboard, adds these updates to the master file and sends that to the outbound
 *		switchboard for distribution to players on that floor.
 *		Takes packets:
 *			IPC_PKT_0 0xB0	PKT_SERVER_SETUP	pktB0 - uses maxplayers
 *			IPC_PKT_1 0xB1	PKT_NEW_CLIENT		pktB1 - uses playerNo
 *							PKT_POS_UPDATE		pkt10 - uses whole packet
 *		Sends packets:
 *							PKT_ALL_POS_UPDATE	pkt11 - uses whole packet
 *
 * Revisions:
 *      -# February 20, 2014: There is currently no error checking, handling or correction in place.  This will come
 *		    in later updates.
 *		    For Milestone 1 only a small part of the functionality is in place.
 *		    Takes packets 10
 *		    Sends packets 11
 *
 *	    -# February 27, 2014: Added error messages for data inconsistencies, send and receive errors
 *		    Now using outbound mask
 *
 * @param[in]   ipcSocks      The Sockets to communicate with other controllers
 * @return void
 *
 * @designer Andrew Burian & Chris Holisky
 * @author Chris Holisky
 *
 * @date February 20, 2014
 */
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

    int updated[MAX_PLAYERS] = {0};

	int maxPlayers = 0;

	OUTMASK m;

	gameplaySock = ((SOCKET*) ipcSocks)[0];
	outswitchSock = ((SOCKET*) ipcSocks)[1];

	//create structs for buffers
	PKT_POS_UPDATE *bufPlayerIn =               malloc(sizeof(PKT_POS_UPDATE));
	PKT_ALL_POS_UPDATE *bufPlayerAll =          malloc(sizeof(PKT_ALL_POS_UPDATE));
	PKT_FLOOR_MOVE_REQUEST *bufFloorMoveReq =   malloc(sizeof(PKT_FLOOR_MOVE_REQUEST));
	PKT_FLOOR_MOVE *bufFloorMove =              malloc(sizeof(PKT_FLOOR_MOVE));

	PKT_MIN_ALL_POS_UPDATE *minAllPos =         malloc(sizeof(PKT_MIN_ALL_POS_UPDATE));
	PKT_MIN_POS_UPDATE *minPos =                malloc(sizeof(PKT_MIN_POS_UPDATE));

	PKT_SERVER_SETUP *bufipcPkt0 =              malloc(sizeof(PKT_SERVER_SETUP));
	PKT_NEW_CLIENT *bufipcPkt1 =                malloc(sizeof(PKT_NEW_CLIENT));
	PKT_LOST_CLIENT *bufipcPkt2 =               malloc(sizeof(PKT_LOST_CLIENT));
	PKT_FORCE_MOVE *bufipcPkt3 =                malloc(sizeof(PKT_FORCE_MOVE));

    // Zero out initial memory
    bzero(bufPlayerIn,      sizeof(PKT_POS_UPDATE));
    bzero(bufPlayerAll,     sizeof(PKT_ALL_POS_UPDATE));
    bzero(bufFloorMoveReq,  sizeof(PKT_FLOOR_MOVE_REQUEST));
    bzero(bufFloorMove,     sizeof(PKT_FLOOR_MOVE));

	bzero(minAllPos,        sizeof(PKT_MIN_ALL_POS_UPDATE));
	bzero(minPos,           sizeof(PKT_MIN_POS_UPDATE));

	bzero(bufipcPkt0, sizeof(ipcPacketSizes[0]));
	bzero(bufipcPkt1, sizeof(ipcPacketSizes[1]));
	bzero(bufipcPkt2, sizeof(ipcPacketSizes[2]));
	bzero(bufipcPkt3, sizeof(ipcPacketSizes[3]));


	//Create array of floor structures
	PKT_ALL_POS_UPDATE floorArray[MAX_FLOORS + 1];
	bzero(floorArray, sizeof(PKT_ALL_POS_UPDATE) * (MAX_FLOORS + 1));
	for (i = 0; i <= MAX_FLOORS; i++) {
		floorArray[i].floor = i;
	}

	getPacketType(gameplaySock);
	getPacket(gameplaySock, bufipcPkt0, ipcPacketSizes[0]);
	maxPlayers = bufipcPkt0->maxPlayers + 1;
	DEBUG(DEBUG_INFO, "GP> Setup Complete");

	while (RUNNING) {
		//get packet type.  Save to int.
		pType = getPacketType(gameplaySock);

		//switch to handle each type of packet
		switch (pType) {
		case -1:
			break;

		case 0xB1: //new player packet
			DEBUG(DEBUG_INFO, "GP> Receive packet 0xB1");

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
			bufFloorMove->newFloor = playerFloor;
			getSpawn(bufipcPkt1->playerNo, playerFloor, &bufFloorMove->xPos, &bufFloorMove->yPos);

			//add player to floor array
			floorArray[0].playersOnFloor[thisPlayer] = 1;

			//send floor change packet
			DEBUG(DEBUG_INFO, "GP> Sending packet 13");

			//set outbound mask for outbound server
			OUT_ZERO(m);
			OUT_SET(m, thisPlayer);
			outPType = 13;

			if (write(outswitchSock, &outPType, sizeof(outPType)) == -1) {
				errOut++;
				fprintf(stderr, "Gameplay Controller - sending to outbound switchboard.  Count:%d\n", errOut);
			}
			if (write(outswitchSock, bufFloorMove, netPacketSizes[13]) == -1) {
				errOut++;
				fprintf(stderr, "Gameplay Controller - sending to outbound switchboard.  Count:%d\n", errOut);
			}
			if (write(outswitchSock, &m, sizeof(OUTMASK)) == -1) {
				errOut++;
				fprintf(stderr, "Gameplay Controller - sending to outbound switchboard.  Count:%d\n", errOut);
			}

			//send packet 11 to players on floor
			DEBUG(DEBUG_INFO, "GP> Sending packet 11 to floor 0");

			outPType = MIN_11;
			floorArray[playerFloor].xPos[thisPlayer] = bufFloorMove->xPos;
			floorArray[playerFloor].yPos[thisPlayer] = bufFloorMove->yPos;
			floorArray[playerFloor].xVel[thisPlayer] = 0;
			floorArray[playerFloor].yVel[thisPlayer] = 0;

			//set outbound mask for outbound server
			OUT_ZERO(m);
			for (i = 0; i < MAX_PLAYERS; i++) {
				if (floorArray[playerFloor].playersOnFloor[i] == 1) {
					OUT_SET(m, i);
				}
			}

			encapsulate_all_pos_update(&floorArray[0], minAllPos);

			if (write(outswitchSock, &outPType, sizeof(outPType)) == -1) {
				errOut++;
				fprintf(stderr, "Gameplay Controller - sending to outbound switchboard.  Count:%d\n", errOut);
			}
			if (write(outswitchSock, minAllPos, netPacketSizes[MIN_11]) == -1) {
				errOut++;
				fprintf(stderr, "Gameplay Controller - sending to outbound switchboard.  Count:%d\n", errOut);
			}
			if (write(outswitchSock, &m, sizeof(OUTMASK)) == -1) {
				errOut++;
				fprintf(stderr, "Gameplay Controller - sending to outbound switchboard.  Count:%d\n", errOut);
			}

			break;

		case 0xB2: //lost or quit client

			DEBUG(DEBUG_INFO, "GP> Received packet 0xB2");

			bzero(bufipcPkt2, ipcPacketSizes[2]);
			if (getPacket(gameplaySock, bufipcPkt2, ipcPacketSizes[2]) == -1) {
				//couldn't read packet
				errPacket++;
				fprintf(stderr, "Gameplay Controller - error reading packet 0xB2.  Count:%d\n", errPacket);
				break;
			}
			for (i = 0; i < MAX_FLOORS + 1; i++) {
				floorArray[i].playersOnFloor[bufipcPkt2->playerNo] = 0;
			}
			break;

		case 0xB3:  // Force floor change

			DEBUG(DEBUG_INFO, "GP> Received packet 0xB3");

			bzero(bufipcPkt3, ipcPacketSizes[3]);

			if (getPacket(gameplaySock, bufipcPkt3, ipcPacketSizes[3]) == -1) {
				//couldn't read packet
				errPacket++;
				fprintf(stderr, "Gameplay Controller - error reading packet 0xB3.  Count:%d\n", errPacket);
				break;
			}

			// Find the player's floor
			for (i = 0; i < MAX_FLOORS; ++i) {
				if (floorArray[i].playersOnFloor[bufipcPkt3->playerNo]) {
					// found player
					break;
				}
			}

			// remove player from floor
			floorArray[i].playersOnFloor[bufipcPkt3->playerNo] = 0;

			// add them to the new floor
			floorArray[bufipcPkt3->newFloor].playersOnFloor[bufipcPkt3->playerNo] = 1;

			// prepare outbound packet
			bufFloorMove->newFloor = bufipcPkt3->newFloor;


			// put them in new locations
			getSpawn(bufipcPkt3->playerNo, bufipcPkt3->newFloor, &bufFloorMove->xPos, &bufFloorMove->yPos);

			// send the player floor move
			outPType = 13;
			OUT_ZERO(m);
			OUT_SET(m, bufipcPkt3->playerNo);
			write(outswitchSock, &outPType, sizeof(packet_t));
			write(outswitchSock, bufFloorMove, netPacketSizes[13]);
			write(outswitchSock, &m, sizeof(OUTMASK));
			DEBUG(DEBUG_INFO, "CP> Sending packet 13");

			//send updated data to all players on old floor
			outPType = MIN_11;

			//set outbound mask for outbound server
			OUT_ZERO(m);
			for (j = 0; j < MAX_PLAYERS; j++) {
				if (floorArray[i].playersOnFloor[j] == 1) {
					OUT_SET(m, j);
				}
			}

			encapsulate_all_pos_update(&floorArray[i], minAllPos);

			write(outswitchSock, &outPType, sizeof(packet_t));
			write(outswitchSock, minAllPos, netPacketSizes[MIN_11]);
			write(outswitchSock, &m, sizeof(OUTMASK));
			DEBUG(DEBUG_INFO, "CP> Sending packet 16");

			//send updated data to all players on new floor
			//set outbound mask for outbound server
			OUT_ZERO(m);
			for (j = 0; j < MAX_PLAYERS; j++) {
				if (floorArray[bufipcPkt3->newFloor].playersOnFloor[j] == 1) {
					OUT_SET(m, j);
				}
			}

			encapsulate_all_pos_update(&floorArray[bufipcPkt3->newFloor], minAllPos);

			write(outswitchSock, &outPType, sizeof(packet_t));
			write(outswitchSock, minAllPos, netPacketSizes[MIN_11]);
			write(outswitchSock, &m, sizeof(OUTMASK));
			DEBUG(DEBUG_INFO, "CP> Sending packet 16");

			break;
        case MIN_10:
            getPacket(gameplaySock, minPos, netPacketSizes[MIN_10]);
            decapsulate_pos_update(minPos, bufPlayerIn);
            /* no break */

		case 10: //player position update

			if(pType == 10){ // Not a min packet
                if (getPacket(gameplaySock, bufPlayerIn, netPacketSizes[10]) == -1) {
                    //couldn't read packet
                    errPacket++;
                    fprintf(stderr, "Gameplay Controller - error reading packet 10.  Count:%d\n", errPacket);
                    break;
                }
            }
			//get floor number from incoming packet
			playerFloor = (bufPlayerIn->floor);

			//check to see if the floor number falls in the valid range
			if (playerFloor < 0 || playerFloor > MAX_FLOORS) {
			    DEBUG(DEBUG_ALRM, "Received pkt 10 with whack floor data");
				break;
			}

			//get player number from incoming packet
			thisPlayer = (*bufPlayerIn).playerNumber;

			//check to see if the player number falls in the valid range
			if (thisPlayer < 0 || thisPlayer > maxPlayers) {
                DEBUG(DEBUG_ALRM, "Received pkt 10 with whack player number data");
				break;
			}

			//is this player supposed to be on this floor?
			if (floorArray[playerFloor].playersOnFloor[thisPlayer] != 1) {
				/*
				 * handle error - this means that the player is sending a
				 * packet with the wrong floor number.  Perhaps pop up a big
				 * message calling them a dirty cheater or just ignore this
				 * packet as corrupt.  If we ignore, we will probably want to
				 * put in a count so that if several bad floor packets come in
				 * from this player we can figure out if we didn't get their
				 * floor update.
				 */
                if(DEBUG_ON && DEBUG_LEVEL >= DEBUG_WARN){
                    errFloor++;
                    fprintf(stderr, "Gameplay Controller - player %d floor incorrect: %d.  Count:%d\n", thisPlayer, playerFloor, errFloor);
                }
                break;
			}

			//put position and velocity in update packet
			floorArray[playerFloor].xPos[thisPlayer] = bufPlayerIn->xPos;
			floorArray[playerFloor].yPos[thisPlayer] = bufPlayerIn->yPos;
			floorArray[playerFloor].xVel[thisPlayer] = bufPlayerIn->xVel;
			floorArray[playerFloor].yVel[thisPlayer] = bufPlayerIn->yVel;

			updated[thisPlayer] = 1;

			break;
		case 0xB4:
            // Timer tick, send update
			//set packet type for outbound server
			outPType = MIN_11;

			DEBUG(DEBUG_INFO, "Sending game update");

			//set outbound mask for outbound server
			for(j = 0; j < MAX_FLOORS + 1; ++j){
                OUT_ZERO(m);
                for (i = 0; i < MAX_PLAYERS; i++) {
                    if (floorArray[j].playersOnFloor[i] == 1) {
                        OUT_SET(m, i);
                    }
                }

                for(i = 0; i < MAX_PLAYERS; ++i){
                    if(!updated[i] && floorArray[j].playersOnFloor[i]){
                        // Player on this floor needs predictive update
                        floorArray[j].xPos[i] += floorArray[j].xVel[i];
                        floorArray[j].yPos[i] += floorArray[j].yVel[i];
                    }
                }

                encapsulate_all_pos_update(&floorArray[j], minAllPos);

                //send packet type and then packet to outbound switchboard
                if (write(outswitchSock, &outPType, sizeof(outPType)) == -1) {
                    errOut++;
                    fprintf(stderr, "Gameplay Controller - sending to outbound switchboard.  Count:%d\n", errOut);
                }
                if (write(outswitchSock, minAllPos, netPacketSizes[MIN_11]) == -1) {
                    errOut++;
                    fprintf(stderr, "Gameplay Controller - sending to outbound switchboard.  Count:%d\n", errOut);
                }
                if (write(outswitchSock, &m, sizeof(OUTMASK)) == -1) {
                    errOut++;
                    fprintf(stderr, "Gameplay Controller - sending to outbound switchboard.  Count:%d\n", errOut);
                }
			}

			for(i = 0; i < MAX_PLAYERS; ++i){
                updated[i] = 0;
			}

			break;
		case 12: //floor change
			DEBUG(DEBUG_INFO, "GP> Received packet 12");

			bzero(bufFloorMoveReq, sizeof(*bufFloorMoveReq));
			if (getPacket(gameplaySock, bufFloorMoveReq, netPacketSizes[12]) == -1) {
				//couldn't read packet
				if(DEBUG_ON && DEBUG_LEVEL >= DEBUG_WARN){
                    errPacket++;
                    fprintf(stderr, "Gameplay Controller - error reading packet 12.  Count:%d\n", errPacket);
				}
				/*
				 *  handle error here.  Perhaps check the size of the packet as well?
				 */
				break;
			}

			//get information from floor move request
			playerFloor = bufFloorMoveReq->current_floor;
			newFloor = bufFloorMoveReq->desired_floor;
			thisPlayer = bufFloorMoveReq->playerNumber;

            if(!floorArray[playerFloor].playersOnFloor[thisPlayer] || (playerFloor == FLOOR_LOBBY && newFloor != FLOOR_LOBBY)){
                DEBUG(DEBUG_WARN, "Player moving off wrong floor or escaping lobby rejected");
                break;
            }

			//set information for floor move
			bzero(bufFloorMove, sizeof(*bufFloorMove));
			bufFloorMove->newFloor = newFloor;
			bufFloorMove->xPos = bufFloorMoveReq->desired_xPos;
			bufFloorMove->yPos = bufFloorMoveReq->desired_yPos;

			DEBUG(DEBUG_INFO, "GP> Sending packet 13");

			//set outbound mask to send only to this player
			OUT_ZERO(m);
			OUT_SET(m, thisPlayer);

			outPType = 13;
			if (write(outswitchSock, &outPType, sizeof(outPType)) == -1) {
				errOut++;
				fprintf(stderr, "Gameplay Controller - sending to outbound switchboard.  Count:%d\n", errOut);
			}
			if (write(outswitchSock, bufFloorMove, netPacketSizes[13]) == -1) {
				errOut++;
				fprintf(stderr, "Gameplay Controller - sending to outbound switchboard.  Count:%d\n", errOut);
			}
			if (write(outswitchSock, &m, sizeof(OUTMASK)) == -1) {
				errOut++;
				fprintf(stderr, "Gameplay Controller - sending to outbound switchboard.  Count:%d\n", errOut);
			}

			DEBUG(DEBUG_INFO, "GP> Sending packet 16 to old floor");

			//send updated data to all players on old floor
			outPType = MIN_11;

			//remove this player from the floor
			floorArray[playerFloor].playersOnFloor[thisPlayer] = 0;

			//set outbound mask for outbound server
			OUT_ZERO(m);
			for (i = 0; i < MAX_PLAYERS; i++) {
				if (floorArray[playerFloor].playersOnFloor[i] == 1) {
					OUT_SET(m, i);
				}
			}

			encapsulate_all_pos_update(&floorArray[playerFloor], minAllPos);

			if (write(outswitchSock, &outPType, sizeof(outPType)) == -1) {
				errOut++;
				fprintf(stderr, "Gameplay Controller - sending to outbound switchboard.  Count:%d\n", errOut);
			}
			if (write(outswitchSock, minAllPos, netPacketSizes[MIN_11]) == -1) {
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
			floorArray[newFloor].playersOnFloor[thisPlayer] = 1;

			//set outbound mask for outbound server
			OUT_ZERO(m);
			for (i = 0; i < MAX_PLAYERS; i++) {
				if (floorArray[newFloor].playersOnFloor[i] == 1) {
					OUT_SET(m, i);
				}
			}
			DEBUG(DEBUG_INFO, "GP> Sending packet 16 to new floor");

			encapsulate_all_pos_update(&floorArray[newFloor], minAllPos);

			if (write(outswitchSock, &outPType, sizeof(outPType)) == -1) {
				errOut++;
				fprintf(stderr, "Gameplay Controller - sending to outbound switchboard.  Count:%d\n", errOut);
			}
			if (write(outswitchSock, minAllPos, netPacketSizes[MIN_11]) == -1) {
				errOut++;
				fprintf(stderr, "Gameplay Controller - sending to outbound switchboard.  Count:%d\n", errOut);
			}
			if (write(outswitchSock, &m, sizeof(OUTMASK)) == -1) {
				errOut++;
				fprintf(stderr, "Gameplay Controller - sending to outbound switchboard.  Count:%d\n", errOut);
			}

			break;
		default:
            if(DEBUG_ON && DEBUG_LEVEL >= DEBUG_WARN){
                wrongPacket++;
                fprintf(stderr, "Gameplay Controller - received unknown packet type:%d  Count:%d\n", pType, wrongPacket);
            }
			break;
		}

		pType = -1;
	}
	free(bufPlayerAll);
	free(bufPlayerIn);
	return 0;
}
