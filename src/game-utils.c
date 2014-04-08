/** @ingroup Server */
/** @{ */

/**
 * Helper utility methods
 *
 * @file game-utils.c
 */

/** @} */

#include "Server.h"


/**
 * Gives a player new coordinates when spawning into a game based on their
 * 	player number so that players do not spawn on top of each other
 *
 * Revisions:
 *      -# None
 *
 * @param[in]   player      	The player number
 * @param[in]   floor      	The new floor number
 * @param[out]  xPos      		The new x position
 * @param[out]  yPos      		The new y position
 * @return void
 *
 * @designer Andrew Burian
 * @author Andrew Burian
 *
 * @date April 2, 2014
 **/
void getSpawn(playerNo_t player, floorNo_t floor, pos_t* xPos, pos_t* yPos){

    switch(floor){

        case 0: // Lobby floor
            *yPos = ((player / 8) * 68) + 268;
            *xPos = ((player % 8) * 40) + (((player % 8) / 4) * 120) + 420;
            break;

        case 1: // Robber Floor
            *yPos = ((player / 4) * 80) + 940;
            *xPos = ((player % 4) * 80) + 2260;
            break;

        case 3: // Cops Floor
            if(player == 15){
                *xPos = 1640; *yPos = 240;
                break;
            }
            *yPos = ((player / 5) * 80) + 100;
            *xPos = ((player % 5) * 80) + 1280;
            break;

        case 9:
            *yPos = ((player / 8) * 80) + 260;
            *xPos = ((player % 8) * 40) + 460;

            break;

        default:
            *xPos = 100;
            *xPos = 100;
            DEBUG(DEBUG_ALRM, "Getting spawn for unhandled floor");
            break;
    }
}
