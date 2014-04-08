/** @ingroup Server */
/** @{ */

/**
 * This file contains methods to help keep TCP connection live when there is no activity
 * 	and test for lost connections
 *
 *
 * @file keep-alive.c
 */

/** @} */

#include "Server.h"

extern int RUNNING;

/**
 * Function sends 'heartbeats' to connected clients when there is no activity for a time,
 * 	also tests to make sure that client is still connected and sends an internal
 * 	lost player message if they are not
 *
 * Revisions:
 *      -# None
 *
 * @param[in]   outSock     Array of TCP sockets
 * @return void
 *
 * @designer German Villarreal
 * @author German Villarreal
 *
 * @date March 20, 2014
 */
void* KeepAlive(void* outSock){
    SOCKET out = ((SOCKET*)outSock)[0];
    SOCKET in = ((SOCKET*)outSock)[1];
    int i = 0;
    packet_t check = KEEP_ALIVE;
    OUTMASK m;
    int send = 0;

    PKT_LOST_CLIENT lost;
    packet_t lostType = IPC_PKT_2;


    while(RUNNING && CHECK_CONNECTIONS){
        sleep(CHECK_FREQUENCY);


        // Send a heartbeat to the client if we haven't sent them anything in a while
        OUT_ZERO(m);
        send = 0;
        for(i = 0; i < MAX_PLAYERS; ++i){
            if(tcpConnections[i] && ((time(NULL) - serverHeartbeat[i]) > CHECK_FREQUENCY)){
               OUT_SET(m, i);
               send = 1;
            }
        }
        if(send){
            write(out, &check, sizeof(packet_t));
            write(out, &m, sizeof(OUTMASK));
        }


        // Check if any of the clients have died
        for(i = 0; i < MAX_PLAYERS; ++i){
            if(tcpConnections[i] && ((time(NULL) - clientHeartbeat[i]) > PRESUME_DEAD_FREQUENCY)){

                bzero(&lost, ipcPacketSizes[2]);

                lost.playerNo = i;

                write(in, &lostType, sizeof(packet_t));
                write(in, &lost, ipcPacketSizes[2]);

            }
        }


    }
    return NULL;
}
