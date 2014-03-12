void* KeepAlive(void* outSock){
    int i = 0;
    packet_t check = 0xFFFFFFFF;
    while(RUNNING){
        sleep(2);
        for(i = 0; i < MAX_PLAYERS; ++i){
            if(tcpConnections[i] && (heartbeats[i]/CLOCKS_PER_SEC > 2){
                write(outSock, &check, sizeof(packet_t));
            }
        }
    }
    return NULL;
}
