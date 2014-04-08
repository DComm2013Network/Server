#include "Server.h"

extern int RUNNING;

/**
 * Sends an alarm packet to the gameplay controller once at MOVE_UPDATE_FREQUENCY
 * times a second, which causes a movement update to be sent to all players in the game
 *
 * Revisions:
 *      -# none
 *
 * @param[in]   ipcSocks    socket to the inbound switchboard
 * @return null
 *
 * @designer Andrew Burian
 * @author Andrew Burian
 *
 * @date March 6, 2014
 */
void* MovementTimer(void* ipcSocks) {
    packet_t setup;
    void* setupPacket = malloc(ipcPacketSizes[0]);

    SOCKET inSwitch = ((SOCKET*)ipcSocks)[0];
    packet_t alarm = 0xB4;
    int sleepTime = 1000000 / MOVE_UPDATE_FREQUENCY;

    // Wait for setup to complete.
    setup = getPacketType(inSwitch);
    if(setup != IPC_PKT_0){
        DEBUG(DEBUG_ALRM, "Timer getting incorrect setup packets");
    }

    getPacket(inSwitch, setupPacket, ipcPacketSizes[0]);

    free(setupPacket);

    // Setup complete, run endless update loop
    while(RUNNING){
        usleep(sleepTime);
        write(inSwitch, &alarm, sizeof(packet_t));
    }


    return NULL;
}
