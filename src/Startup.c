/** @ingroup Server */
/** @{ */

/**
 * This file contains the execution entry point for Cut the Power server. Functions consist of
 * overall application setup and constant initilizations. Packet sizes and general control
 * variables initialized and application structure set.
 *
 * @file Startup.c
 */

/** @} */

#include "Server.h"

#define READ 0
#define WRITE 1

#define NUM_CONTROLLERS 8

// Super Global
int RUNNING = 1;

int KillHandler(int signo);

/**
 * Initializes global array consisting of the network packet sizes, and the
 * ipc packet sizes. Also finds the largest packet of each type.
 *
 * Revisions:
 *      -# none.
 *
 * @param[in]   ipcSocks     Array of 2 sockets for reading and writing.
 * @return void
 *
 * @designer Andrew Burian
 * @author Andrew Burian
 *
 * @date February 1, 2014
 */
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
	netPacketSizes[7] = 0;
	netPacketSizes[8] = sizeof(struct pkt08);
	netPacketSizes[9] = 0; // keep alive data is 0
	netPacketSizes[10] = sizeof(struct pkt10);
	netPacketSizes[11] = sizeof(struct pkt11);
	netPacketSizes[12] = sizeof(struct pkt12);
	netPacketSizes[13] = sizeof(struct pkt13);
	netPacketSizes[14] = sizeof(struct pkt14);
	netPacketSizes[15] = sizeof(struct pkt15);
	netPacketSizes[16] = sizeof(struct pkt16);

	for(i = 0; i < NUM_NET_PACKETS + 1; ++i){
		largestNetPacket = (netPacketSizes[i] > largestNetPacket) ? netPacketSizes[i] : largestNetPacket;
	}


	ipcPacketSizes[0] = sizeof(struct pktB0);
	ipcPacketSizes[1] = sizeof(struct pktB1);
	ipcPacketSizes[2] = sizeof(struct pktB2);
	ipcPacketSizes[3] = sizeof(struct pktB3);
	ipcPacketSizes[4] = 0; // alarm packet has no data

	for(i = 0; i < NUM_IPC_PACKETS + 1; ++i){
		largestIpcPacket = (ipcPacketSizes[i] > largestIpcPacket) ? ipcPacketSizes[i] : largestIpcPacket;
	}


	largestPacket = ((largestIpcPacket > largestNetPacket) ? largestIpcPacket : largestNetPacket);

}

/**
 * Execution entry point of the terminal server application. Launches each controller as a thread
 * with its needed sockets for inter process communication.
 *
 * Revisions:
 *      -# none.
 *
 * @param[in]   argc    Number of command line arguments.
 * @param[in]   argv    Array of command line arguments.
 * @return void
 *
 * @designer Andrew Burian
 * @author Andrew Burian
 *
 * @date February 1, 2014
 */
int main(int argc, char* argv[]) {
	SOCKET uiSockSet[2];
	SOCKET connectionSockSet[2];
	SOCKET generalSockSet[2];
	SOCKET gameplaySockSet[2];
	SOCKET keepAliveSockSet[2];
	SOCKET timerSockSet[2];

	SOCKET out_in[2];
	SOCKET out_gen[2];
	SOCKET out_game[2];
	SOCKET out_keepal[2];

	SOCKET uiParams[1];
	SOCKET conManParams[1];
	SOCKET generalParams[2];
	SOCKET gameplayParams[2];
	SOCKET outboundParams[4];
	SOCKET inboundParams[6];
	SOCKET keepAliveParams[2];
	SOCKET timerParams[1];

	pthread_t controllers[NUM_CONTROLLERS];
	int threadResult = 0;
	int i = 0;

	printf("%s\n%s\n%s%.1lf\n%s\n\n",
		"-------------------------------------------------------------",
		"                     Cut The Power                           ",
		"                      Server v", SERVER_VERSION,
		"-------------------------------------------------------------");

	DEBUG(DEBUG_ALRM, "Started With Debug");
	DEBUG(DEBUG_ALRM, " - Alarms [on]");
	DEBUG(DEBUG_WARN, " - Warnings [on]");
	DEBUG(DEBUG_INFO, " - Information [on]");

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
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, out_keepal) == -1) {
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

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, keepAliveSockSet) == -1) {
		fprintf(stderr, "Socket pair error: keepAliveSockSet");
		return -1;
	}

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, timerSockSet) == -1) {
		fprintf(stderr, "Socket pair error: timerSockSet");
		return -1;
	}

	DEBUG(DEBUG_INFO, "All IPC sockets created succesfully");



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
	outboundParams[3] = out_keepal[READ];
	threadResult += pthread_create(&controllers[4], NULL, OutboundSwitchboard, (void*)outboundParams);
	// ----------------------------



    	// Start the Keep Alive Cleaner
	keepAliveParams[0] = out_keepal[WRITE];
	keepAliveParams[1] = keepAliveSockSet[WRITE];
	threadResult += pthread_create(&controllers[5], NULL, KeepAlive, (void*)keepAliveParams);
	// ----------------------------


    	// Start the Movement Timer
    	timerParams[0] = timerSockSet[WRITE];
	threadResult += pthread_create(&controllers[6], NULL, MovementTimer, (void*)timerParams);
	// ----------------------------


	// Start the Inbound Switchboard
	inboundParams[0] = generalSockSet[WRITE];
	inboundParams[1] = gameplaySockSet[WRITE];
	inboundParams[2] = uiSockSet[READ];
	inboundParams[3] = out_in[WRITE];
	inboundParams[4] = connectionSockSet[READ];
	inboundParams[5] = keepAliveSockSet[READ];
	inboundParams[6] = timerSockSet[READ];
	threadResult += pthread_create(&controllers[7], NULL, InboundSwitchboard, (void*)inboundParams);
	// ----------------------------


	if(threadResult<0){
		fprintf(stderr, "One or more controllers failed to launch!\n Terminating.");

		// Kill all launched threads
		for(i = 0; i < NUM_CONTROLLERS; ++i){
			pthread_kill(controllers[i], SIGKILL);
		}

	}
	else{
		DEBUG(DEBUG_INFO, "All controllers launched");

		// Wait on the inbound switchboard to terminate process
		pthread_join(controllers[6], NULL);
	}


	printf("%s\n%s\n\n",
		"-------------------------------------------------------------",
		"                   Server Terminated\n");
	return 0;
}

/**
 * Signal handler, changes the server's running state allowing all threads
 * to clean up and exit.
 *
 * Revisions:
 *      -# none.
 *
 * @param[in]   signo   Signal read
 *
 * @return int
 *
 * @designer Andrew Burian
 * @author Andrew Burian
 *
 * @date February 1, 2014
 */
int KillHandler(int signo){
    printf("Server Terminated by user keystroke!");
    RUNNING = 0;
    return 0;
}

