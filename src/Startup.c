/*---------------------------------------------------------------------*
 -- HEADER FILE: Startup.c 	The main entry point for the server process
 --
 --
 -- PROGRAM:		Game-Server
 --
 -- FUNCTIONS:
 -- 		int main(int argc, char* argv[])
 --
 -- DATE: 		January 30, 2014
 --
 -- REVISIONS: 	none
 --
 -- DESIGNER: 	Andrew Burian
 --
 -- PROGRAMMER: 	Andrew Burian
 --
 -- NOTES:
 --
 ----------------------------------------------------------------------*/

/*
 *
 *
 *     START
 setup socket pair for ui
 capture sigint
 fork ui

 SETUP
 wait for "setup packet" from ui
 initialize

 CLEANUP
 sigint all processes
 close all connections
 */
#include "NetComm.h"
#include "Server.h"

#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#define READ 0
#define WRITE 1

// Super Global
int RUNNING = 1;

int main(int argc, char* argv[]) {
	SOCKET uiSockSet[2];
	SOCKET connectionSockSet[2];
	SOCKET generalSockSet[2];
	SOCKET gameplaySockSet[2];
	SOCKET outswitchSockSet[2];

	DEBUG("Started");
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, uiSockSet) == -1) {
		fprintf(stderr, "Socket pair error: uiSockSet");
		return -1;
	}

	// Start the UI process
	if (fork() == 0) {
		UI(uiSockSet[WRITE]);
		return 0;
	}
	close(uiSockSet[WRITE]);
	// ----------------------------

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, outswitchSockSet) == -1) {
		fprintf(stderr, "Socket pair error: outswitchSockSet");
		return -1;
	}
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, connectionSockSet) == -1) {
		fprintf(stderr, "Socket pair error: connectionSockSet");
		return -1;
	}

	// Start the Connection Manager
	if (fork() == 0) {
		ConnectionManager(connectionSockSet[READ], outswitchSockSet[WRITE]);
		return 0;
	}
	close(connectionSockSet[READ]);
	// ----------------------------

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, generalSockSet) == -1) {
		fprintf(stderr, "Socket pair error: generalSockSet");
		return -1;
	}

	// Start the General Controller
	if (fork() == 0) {
		GeneralController(generalSockSet[READ], outswitchSockSet[WRITE]);
		return 0;
	}
	close(generalSockSet[READ]);
	// ----------------------------

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, gameplaySockSet) == -1) {
		fprintf(stderr, "Socket pair error: gameplaySockSet");
		return -1;
	}

	// Start the Gameplay Controller
	if (fork() == 0) {
		GameplayController(gameplaySockSet[READ], outswitchSockSet[WRITE]);
		return 0;
	}
	close(gameplaySockSet[READ]);
	// ----------------------------

	// Start the Outbound Switchboard
	if (fork() == 0) {
		OutboundSwitchboard(outswitchSockSet[READ]);
		return 0;
	}
	close(gameplaySockSet[READ]);
	// ----------------------------

	InboundSwitchboard(connectionSockSet[WRITE], generalSockSet[WRITE],
			gameplaySockSet[WRITE], outswitchSockSet[WRITE]);
	
	printf("Server Terminated\n");
	return 0;
}

