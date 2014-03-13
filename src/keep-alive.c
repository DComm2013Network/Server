#include "Server.h"

extern int RUNNING;

void* KeepAlive(void* outSock){
    SOCKET out = *((SOCKET*)outSock);
    int i = 0;
    packet_t check = KEEP_ALIVE;
    OUTMASK m;
    int send = 0;
    while(RUNNING && CHECK_CONNECTIONS){
        sleep(CLEANUP_FREQUENCY);
        OUT_ZERO(m);
        send = 0;
        for(i = 0; i < MAX_PLAYERS; ++i){
            if(tcpConnections[i] && (heartbeats[i]/CLOCKS_PER_SEC > CLEANUP_FREQUENCY)){
               OUT_SET(m, i);
               send = 1;
            }
        }
        if(send){
            write(out, &check, sizeof(packet_t));
            write(out, &m, sizeof(OUTMASK));
        }
    }
    return NULL;
}
