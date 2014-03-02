/*---------------------------------------------------------------------*
 -- SOURCE FILE: Startup.c 	The main entry point for the server process
 --
 --
 -- PROGRAM:		Game-Server
 --
 -- FUNCTIONS:
 -- 		int main(int argc, char* argv[])
 --
 -- DATE: 			January 30, 2014
 --
 -- REVISIONS: 		none
 --
 -- DESIGNER: 		Andrew Burian
 --
 -- PROGRAMMER: 	Andrew Burian
 --
 -- NOTES:
 --
 ----------------------------------------------------------------------*/

#include "Server.h"

#define READ 0
#define WRITE 1

#define NUM_CONTROLLERS 6

// Super Global
int RUNNING = 1;

int KillHandler(int signo);


void setupPacketInfo(){

	int i;

	largestNetPacket = 0;
	largestIpcPacket = 0;

	netPacketSizes[0] = 0;
	netPacketSizes[1] = sizeof(struct pkt01);
	netPacketSizes[2] = sizeof(struct pkt02);
	netPacketSizes[3] = sizeof(struct pkt03);
	netPacketSizes[4] = sizeof(struct pkt04);
	netPacketSizes[5] = sizeof(struct pkt05);
	netPacketSizes[6] = sizeof(struct pkt06);
	netPacketSizes[7] = sizeof(struct pkt07);
	netPacketSizes[8] = sizeof(struct pkt08);
	netPacketSizes[9] = sizeof(struct pkt09);
	netPacketSizes[10] = sizeof(struct pkt10);
	netPacketSizes[11] = sizeof(struct pkt11);
	netPacketSizes[12] = sizeof(struct pkt12);
	netPacketSizes[13] = sizeof(struct pkt13);

	for(i = 0; i < NUM_NET_PACKETS + 1; ++i){
		largestNetPacket = (netPacketSizes[i] > largestNetPacket) ? netPacketSizes[i] : largestNetPacket;
	}


	ipcPacketSizes[0] = sizeof(struct pktB0);
	ipcPacketSizes[1] = sizeof(struct pktB1);
	ipcPacketSizes[2] = sizeof(struct pktB2);

	for(i = 0; i < NUM_IPC_PACKETS + 1; ++i){
		largestIpcPacket = (ipcPacketSizes[i] > largestIpcPacket) ? ipcPacketSizes[i] : largestIpcPacket;
	}


	largestPacket = ((largestIpcPacket > largestNetPacket) ? largestIpcPacket : largestNetPacket);

}










int main(int argc, char* argv[]) {
	SOCKET uiSockSet[2];
	SOCKET connectionSockSet[2];
	SOCKET generalSockSet[2];
	SOCKET gameplaySockSet[2];

	SOCKET out_in[2];
	SOCKET out_gen[2];
	SOCKET out_game[2];

	SOCKET uiParams[1];
	SOCKET conManParams[2];
	SOCKET generalParams[2];
	SOCKET gameplayParams[2];
	SOCKET outboundParams[3];
	SOCKET inboundParams[5];

	pthread_t controllers[NUM_CONTROLLERS];
	int threadResult = 0;
	int i = 0;

	printf("%s\n%s\n%s%.1lf\n%s\n\n",
		"-------------------------------------------------------------",
		"                     Cut The Power                           ",
		"                      Server v", SERVER_VERSION,
		"-------------------------------------------------------------");

	DEBUG("Started With Debug");

    //signal(SIGINT, KillHandler);

	setupPacketInfo();

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, uiSockSet) == -1) {
		fprintf(stderr, "Socket pair error: uiSockSet");
		return -1;
	}

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, out_in) == -1) {
		fprintf(stderr, "Socket pair error: outswitchSockSet");
		return -1;
	}
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, out_game) == -1) {
		fprintf(stderr, "Socket pair error: outswitchSockSet");
		return -1;
	}
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, out_gen) == -1) {
		fprintf(stderr, "Socket pair error: outswitchSockSet");
		return -1;
	}

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, connectionSockSet) == -1) {
		fprintf(stderr, "Socket pair error: connectionSockSet");
		return -1;
	}

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, generalSockSet) == -1) {
		fprintf(stderr, "Socket pair error: generalSockSet");
		return -1;
	}

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, gameplaySockSet) == -1) {
		fprintf(stderr, "Socket pair error: gameplaySockSet");
		return -1;
	}

	DEBUG("All IPC sockets created succesfully");



	// Start UI Controller
	uiParams[0] = uiSockSet[WRITE];
	threadResult += pthread_create(&controllers[0], NULL, UIController, (void*)uiParams);
	// ----------------------------


	// Start Connection Manager
	conManParams[0] = connectionSockSet[WRITE];
	threadResult += pthread_create(&controllers[1], NULL, ConnectionManager, (void*)conManParams);
	// ----------------------------



	// Start the General Controller
	generalParams[0] = generalSockSet[READ];
	generalParams[1] = out_gen[WRITE];
	threadResult += pthread_create(&controllers[2], NULL, GeneralController, (void*)generalParams);
	// ----------------------------


	// Start the Gameplay Controller
	gameplayParams[0] = gameplaySockSet[READ];
	gameplayParams[1] = out_game[WRITE];
	threadResult += pthread_create(&controllers[3], NULL, GameplayController, (void*)gameplayParams);
	// ----------------------------


	// Start the Outbound Switchboard
	outboundParams[0] = out_in[READ];
	outboundParams[1] = out_game[READ];
	outboundParams[2] = out_gen[READ];
	threadResult += pthread_create(&controllers[4], NULL, OutboundSwitchboard, (void*)outboundParams);
	// ----------------------------



	// Start the Inbound Switchboard
	inboundParams[0] = generalSockSet[WRITE];
	inboundParams[1] = gameplaySockSet[WRITE];
	inboundParams[2] = uiSockSet[READ];
	inboundParams[3] = out_in[WRITE];
	inboundParams[4] = connectionSockSet[READ];
	threadResult += pthread_create(&controllers[5], NULL, InboundSwitchboard, (void*)inboundParams);
	// ----------------------------


	if(threadResult<0){
		fprintf(stderr, "One or more controllers failed to launch!\n Terminating.");

		// Kill all launched threads
		for(i = 0; i < NUM_CONTROLLERS; ++i){
			pthread_kill(controllers[i], SIGKILL);
		}

	}
	else{
		DEBUG("All controllers launched");

		// Wait on the inbound switchboard to terminate process
		pthread_join(controllers[5], NULL);
	}


	printf("%s\n%s\n\n",
		"-------------------------------------------------------------",
		"                   Server Terminated\n");
	return 0;
}


int KillHandler(int signo){
    printf("Server Terminated by user keystroke!");
    RUNNING = 0;
    return 0;
}

