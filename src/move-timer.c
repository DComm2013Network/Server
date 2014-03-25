#include "Server.h"

extern int RUNNING;

void* MovementTimer(void* ipcSocks) {
    packet_t setup;
    void* setupPacket = malloc(ipcPacketSizes[0]);

    SOCKET inSwitch = ((SOCKET*)ipcSocks)[0];
    packet_t alarm = 0xB4;
    int sleepTime = 1000000 / MOVE_UPDATE_FREQUENCY;

    // Wait for setup to complete.
    setup = getPacketType(inSwitch);
    if(setup != IPC_PKT_0){
        DEBUG("Timer getting incorrect setup packets");
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
