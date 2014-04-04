/*-------------------------------------------------------------------------------------------------------------------*
-- SOURCE FILE: ConnectionManager.c
--		The Process that will deal with new connections being added, and potentially reconnecting lost ones.
--
-- FUNCTIONS:
-- 		int ConnectionManager(SOCKET connectionSock, SOCKET outswitchSock)
--		void addNewConnection(int maxPlayers, SOCKET connectionSock, SOCKET outswitchSock)
-- 		void removeConnection(SOCKET connectionSock)
--		void connectionManagerSetup()
--
--
-- DATE: 		February 16, 2014
--
-- REVISIONS: 	none
--
-- DESIGNER: 	Andrew Burian
--
-- PROGRAMMER: 	Andrew Burian
--
-- NOTES:
--
*-------------------------------------------------------------------------------------------------------------------*/

#include "Server.h"
#include <ctype.h>

int checkName(char name[MAX_NAME]);
void makeRandomName(char name[MAX_NAME]);

// Globals
SOCKET listenSock;
extern int RUNNING;


/*--------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	Setup
--
-- DATE: 		February 16, 2014
--
-- REVISIONS: 	none
--
-- DESIGNER: 	Andrew Burian
--
-- PROGRAMMER: 	Andrew Burian
--
-- INTERFACE: 	void connectionManagerSetup(SOCKET connectionSock, int* maxPlayers, char* gameName)
--
-- RETURNS: 	void
--
-- NOTES:
-- Reads in the setup packet send by the switchboard from the UI
----------------------------------------------------------------------------------------------------------------------*/
void connectionManagerSetup(SOCKET connectionSock, int* maxPlayers, char* gameName){

	struct pktB0 setupPkt;

	if(getPacketType(connectionSock) != 0xB0){
		DEBUG(DEBUG_WARN, "CM> CM setup getting packets it shouldn't be");
		return;
	}

	getPacket(connectionSock, &setupPkt, ipcPacketSizes[0]);

	*maxPlayers = setupPkt.maxPlayers;
	memcpy(gameName, setupPkt.serverName, MAX_NAME);

	DEBUG(DEBUG_INFO, "CM> Setup Complete");

	return;
}

/*--------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	Add New Connection
--
-- DATE: 		February 16, 2014
--
-- REVISIONS: 	none
--
-- DESIGNER: 	Andrew Burian
--
-- PROGRAMMER: 	Andrew Burian
--
-- INTERFACE: 	void addNewConnection(int maxPlayers, SOCKET connectionSock, SOCKET outswitchSock)
--
-- RETURNS: 	void
--
-- NOTES:
-- Tries to add a new player to the server. If successful, notifies Switchboards.
-- If the server is full, client is rejected, and the connection is closed.
----------------------------------------------------------------------------------------------------------------------*/
void addNewConnection(int maxPlayers, SOCKET connectionSock){

	struct pktB1 newClientInfo;
	struct pkt01 clientReg;
	struct pkt02 replyToClient;

	packet_t replyType = 0x02;
	packet_t newClientType = 0xB1;

	SOCKET acceptSock;

	struct sockaddr_in client;
	socklen_t addr_len = sizeof(struct sockaddr_in);
	int i;

	bzero(&newClientInfo, ipcPacketSizes[1]);
	bzero(&clientReg, netPacketSizes[1]);
	bzero(&replyToClient, netPacketSizes[2]);

	DEBUG(DEBUG_INFO,"CM> Adding new connection");

	bzero(&replyToClient, netPacketSizes[2]);
	bzero(&newClientInfo, ipcPacketSizes[1]);

	if ((acceptSock = accept(listenSock, (struct sockaddr *)&client, &addr_len)) == -1){
		DEBUG(DEBUG_ALRM, "Could not accept client");
		return;
	}

	// Read in the packet type, which should be 1
	if(getPacketType(acceptSock) != 1){
		DEBUG(DEBUG_WARN, "CM> Add new connection getting packets it shouldn't be");
		return;
	}

	// Register the new client if there is space available
	getPacket(acceptSock, &clientReg, netPacketSizes[1]);

	for(i = 0; i < maxPlayers; ++i){
		if(tcpConnections[i] == 0){
			break;
		}
	}

	// Add player if space
	if(i < maxPlayers){
		DEBUG(DEBUG_INFO, "CM> Space available, adding player");

        connectedPlayers++;
		replyToClient.connectCode = connectCode_ACCEPTED;
		replyToClient.clients_playerNumber = i;

		if(!checkName(clientReg.playerName)){
            makeRandomName(clientReg.playerName);
        }

        // give the player either his name back or the revised name
        memcpy(replyToClient.playerName, clientReg.playerName, MAX_NAME);

		// The client's inital team number is 0. This will be later assigned by the Conn Man
		replyToClient.clients_team_number = 0;

		// Send accept to client with their player and team number
		send(acceptSock, &replyType, sizeof(packet_t), 0);
		send(acceptSock, &replyToClient, sizeof(struct pkt02), 0);
		DEBUG(DEBUG_INFO, "CM> Sent pkt 2");

		newClientInfo.playerNo = i;
		memcpy(newClientInfo.playerName, clientReg.playerName, MAX_NAME);
		newClientInfo.character = clientReg.selectedChatacter;

		// add TCP connection to list
		tcpConnections[i] = acceptSock;
		clientPulse(i);

		// set UDP connection info, but override the port
		memcpy(&udpAddresses[i], &client, sizeof(struct sockaddr_in));
		udpAddresses[i].sin_port = htons(UDP_PORT);

		DEBUG(DEBUG_INFO, "CM> Notifying switchboards");
		// Notify switchboards
		write(connectionSock, &newClientType, sizeof(packet_t));
		write(connectionSock, &newClientInfo, sizeof(struct pktB1));

		DEBUG(DEBUG_INFO, "CM> New client added");
	}
	else{
		// Server is full. GTFO
		DEBUG(DEBUG_WARN, "CM> Game full, rejecting client");
		replyToClient.connectCode = connectCode_DENIED;
		send(acceptSock, &replyType, sizeof(packet_t), 0);
		send(acceptSock, &replyToClient, sizeof(struct pkt02), 0);
		close(acceptSock);
	}

	return;
}


/*--------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	Remove Connection
--
-- DATE: 		February 16, 2014
--
-- REVISIONS: 	none
--
-- DESIGNER: 	Andrew Burian
--
-- PROGRAMMER: 	Andrew Burian
--
-- INTERFACE: 	void removeConnection()
--
-- RETURNS: 	void
--
-- NOTES:
--
----------------------------------------------------------------------------------------------------------------------*/
void removeConnection(SOCKET connectionSock){
	// No need to send the remove to outswitch, inswich will have done it

	struct pktB2 lostClient;
	int type = 0xB2;

	DEBUG(DEBUG_WARN, "CM> Removing player from game");

	if(getPacketType(connectionSock) != type){
		DEBUG(DEBUG_ALRM, "CM> Connection manager Remove Connection getting packets it shouldn't be.");
		return;
	}

	getPacket(connectionSock, &lostClient, ipcPacketSizes[2]);

	connectedPlayers--;

	DEBUG(DEBUG_INFO, "CM> Removed player from game");

	return;
}


/*--------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	Connection Manager
--
-- DATE: 		February 16, 2014
--
-- REVISIONS: 	none
--
-- DESIGNER: 	Andrew Burian
--
-- PROGRAMMER: 	Andrew Burian
--
-- INTERFACE: 	int ConnectionManager(SOCKET connectionSock, SOCKET outswitchSock)
--
-- RETURNS: 	int
--					failure:	-99 Not yet implemented
--								-1  Socket setup Failed
--					success: 	0
--
-- NOTES:
--
----------------------------------------------------------------------------------------------------------------------*/
void* ConnectionManager(void* ipcSocks){

	int maxPlayers;
	char* gameName;

	int enable = 1;

	fd_set fdset;
	int numLiveSockets;
	SOCKET highSocket;

	SOCKET connectionSock = ((SOCKET*)ipcSocks)[0];

	struct	sockaddr_in server;
	int addr_len = sizeof(struct sockaddr_in);

	gameName = (char*)malloc(sizeof(char) * MAX_NAME);

	DEBUG(DEBUG_INFO, "CM> Connection Manager Started");

	// Read the packet from UI and get started
	connectionManagerSetup(connectionSock, &maxPlayers, gameName);

	// Start listening for incoming connections
	listenSock = 	socket(AF_INET, SOCK_STREAM, 0);
	udpConnection = socket(AF_INET, SOCK_DGRAM, 0);

	// No clients yet joined
	connectedPlayers = 0;
	bzero(tcpConnections, 		sizeof(SOCKET) * MAX_PLAYERS);
	bzero(udpAddresses, 		sizeof(struct sockaddr_in) * MAX_PLAYERS);

	bzero(&server, addr_len);

	// Set up the listening socket to any incoming address
	server.sin_family 		= AF_INET;
	server.sin_port 		= htons(TCP_PORT);
	server.sin_addr.s_addr 	= htonl(INADDR_ANY);

	// Bind
	setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
	if (bind(listenSock, (struct sockaddr *)&server, addr_len) == -1)
	{
		perror("Can't bind listen socket");
		return NULL;
	}

	DEBUG(DEBUG_INFO, "CM> TCP socket bound");

	server.sin_port = htons(UDP_PORT);

	if (bind(udpConnection, (struct sockaddr *)&server, addr_len) == -1)
	{
		perror("Can't bind udp socket");
		return NULL;
	}

	DEBUG(DEBUG_INFO, "CM> UDP socket bound");

	// queue up to 5 connect requests
	listen(listenSock, 5);

	DEBUG(DEBUG_INFO, "CM> Listening");

	while(RUNNING){

		FD_ZERO(&fdset);
		FD_SET(connectionSock, &fdset);
		FD_SET(listenSock, &fdset);
		highSocket = (connectionSock > listenSock) ? connectionSock : listenSock;

		// Find all active Sockets
		numLiveSockets = select(highSocket + 1, &fdset, NULL, NULL, NULL);

		if(numLiveSockets == -1){
			perror("Select Failed in Connection Manager!");
			continue;
		}

		if(FD_ISSET(connectionSock, &fdset)){
			// connection lost
			removeConnection(connectionSock);
		}

		if(FD_ISSET(listenSock, &fdset)){
			// New connection appeared on listen
			addNewConnection(maxPlayers, connectionSock);
		}
	}

	return NULL;
}

int checkName(char name[MAX_NAME]){

    int i;

    // must be at least 3 long
    if(strlen(name) < 3){
        return 0;
    }

    // first letter cannot be a space
    if(isspace(name[0])){
        return 0;
    }

    // has to not be all spaces
    for(i = 0; i < MAX_NAME; ++i){
        if(!isspace(name[i])){
            break;
        }
    }
    if(i == MAX_NAME){
        return 0;
    }

    // ban non-standard ascii characters
    for(i = 0; i < strlen(name); ++i){
        if(name[i] < 33 || name[i] > 126){
            return 0;
        }
    }

    return 1;
}

void makeRandomName(char name[MAX_NAME]){

    int names = 16; // num cases + 1

    bzero(name, MAX_NAME);
    srand(time(0));

    switch(rand() % names){
        case 0:
            snprintf(name, MAX_NAME, "Bandy Avocado");
            break;
        case 1:
            snprintf(name, MAX_NAME, "Fredrick");
            break;
        case 2:
            snprintf(name, MAX_NAME, "J-Beibs");
            break;
        case 3:
            snprintf(name, MAX_NAME, "Gandalf Orange");
            break;
        case 4:
            snprintf(name, MAX_NAME, "A Dancing bear");
            break;
        case 5:
            snprintf(name, MAX_NAME, "Autocratic tar");
            break;
        case 6:
            snprintf(name, MAX_NAME, "Johnny Bravo");
            break;
        case 7:
            snprintf(name, MAX_NAME, "missingName");
            break;
        case 8:
            snprintf(name, MAX_NAME, "Failed grade 4");
            break;
        case 9:
            snprintf(name, MAX_NAME, "Assquatch");
            break;
        case 10:
            snprintf(name, MAX_NAME, "Frodo Swaggins");
            break;
        case 11:
            snprintf(name, MAX_NAME, "SUBTLE AS F*CK");
            break;
        case 12:
            snprintf(name, MAX_NAME, "Casual Server");
            break;
        case 13:
            snprintf(name, MAX_NAME, "Dick Chow");
            break;
        case 14:
            snprintf(name, MAX_NAME, "The real Sam");
            break;
        case 15:
            snprintf(name, MAX_NAME, "Not slim-shady");
            break;
        case 16:
            snprintf(name, MAX_NAME, "Mike Hunt");
            break;

    }
            //--------------------------------------

}
