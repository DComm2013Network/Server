#include "Server.h"

void getSpawn(playerNo_t player, floorNo_t floor, pos_t* xPos, pos_t* yPos){

    switch(floor){

        case 0: // Lobby floor
            *yPos = ((player / 8) * 80) + 240;
            *xPos = ((player % 8) * 40) + 440;
            break;

        case 1: // Robber Floor
            *yPos = ((player / 4) * 80) + 920;
            *xPos = ((player % 4) * 80) + 2260;
            break;

        case 3: // Cops Floor
            if(player == 15){
                *xPos = 1640; *yPos = 240;
                break;
            }
            *yPos = ((player / 5) * 80) + 80;
            *xPos = ((player % 5) * 80) + 1280;
            break;

        default:
            *xPos = 100;
            *xPos = 100;
            DEBUG(DEBUG_ALRM, "Getting spawn for unhandled floor");
            break;
    }
}
