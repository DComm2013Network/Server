#include "Server.h"

time_t gameStart;
status_t currentState;

void chatGameStart(){
    gameStart = time(NULL);
}

void sendChat(PKT_CHAT* chat, teamNo_t teams[MAX_PLAYERS], SOCKET outswitch){

    OUTMASK outM;
    OUT_ZERO(outM);
    int i;
    packet_t type = 4;

    for(i = 0; i < MAX_PLAYERS; ++i){
        if(i != chat->sendingPlayer && teams[i] == teams[chat->sendingPlayer]){
            OUT_SET(outM, i);
        }
    }

    write(outswitch, &type, sizeof(packet_t));
    write(outswitch, chat, netPacketSizes[type]);
    write(outswitch, &outM, sizeof(OUTMASK));

}
