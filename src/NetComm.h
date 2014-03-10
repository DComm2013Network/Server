/*---------------------------------------------------------------------*
-- HEADER FILE: NetComm.h 		The definitions and declarations to be
-- 								to be used to communicate between the
-- 								client and the server
--
-- PROGRAM:		Game-Client/Server
--
-- FUNCTIONS:
-- 		none
--
-- DATE: 		January 27, 2014
--
-- REVISIONS: 	none
--
-- DESIGNER: 	Andrew Burian
--
-- PROGRAMMER: 	Andrew Burian
--
-- NOTES:
-- This MUST be consistant between the client and server
----------------------------------------------------------------------*/

// Limits
#define MAX_PLAYERS 	32
#define MAX_FLOORS		8
#define MAX_NAME	 	80
#define MAX_MESSAGE		180
#define MAX_OBJECTIVES	16

#define NUM_NET_PACKETS 13


// Other Includes
#include <time.h>
#include <stdint.h>

// Definitions for various game data types
typedef uint32_t    floorNo_t;
typedef uint32_t    playerNo_t;
typedef uint32_t    teamNo_t;
typedef uint32_t    status_t;
typedef uint32_t    pos_t;
typedef float	    vel_t;
typedef uint32_t    packet_t;
typedef uint64_t    timestamp_t;


// Connect code Definitions
#define CONNECT_CODE_ACCEPTED	0x001
#define CONNECT_CODE_DENIED		0x000

// Game Status Definitions
#define GAME_STATE_WAITING		0x001
#define GAME_STATE_ACTIVE		0x002
#define GAME_STATE_OVER			0x003
#define PLAYER_STATE_READY		0x004
#define PLAYER_STATE_WAITING	0x005
#define GAME_TEAM1_WIN			0x006
#define GAME_TEAM2_WIN			0x007

// Special floor Definitions
#define FLOOR_LOBBY				0x000

#ifndef PACKETS
#define PACKETS
// Packet Definitions

typedef struct pkt01{
	char 		client_player_name[MAX_NAME];
} PKT_PLAYER_JOIN;

typedef struct pkt02{
	status_t 	connect_code;
	playerNo_t 	clients_player_number;
	teamNo_t 	clients_team_number;
} PKT_JOIN_RESPONSE;

typedef struct pkt03{
	int 		player_valid[MAX_PLAYERS];
	char 		otherPlayers_name[MAX_PLAYERS][MAX_NAME];
	teamNo_t 	otherPlayers_teams[MAX_PLAYERS];
	status_t	readystatus[MAX_PLAYERS];
} PKT_PLAYERS_UPDATE;

typedef struct pkt04{
	playerNo_t 	sendingPlayer_number;
	char 		message[MAX_MESSAGE];
} PKT_CHAT;

typedef struct pkt05{
	playerNo_t	player_number;
	status_t	ready_status;
	teamNo_t	team_number;
	char 		player_name[MAX_NAME];
} PKT_READY_STATUS;

typedef struct pkt06{
	floorNo_t	map_data[MAX_FLOORS];
	int			objective_locations[MAX_OBJECTIVES];
} PKT_OBJECTIVES_DATA;

//Packet 7: 0x0007
//	<< UNPURPOSED >>

typedef struct pkt08{
	int		    objectives_captured[MAX_OBJECTIVES];
	status_t	game_status;
} PKT_GAME_STATUS;

//Packet 9: 0x0009
//	<< UNPURPOSED >>

typedef struct pkt10{
	floorNo_t 	floor;
	playerNo_t 	player_number;
	pos_t 		xPos;
	pos_t		yPos;
	vel_t		xVel;
	vel_t		yVel;
} PKT_POS_UPDATE;

typedef struct pkt11{
	floorNo_t 	floor;
	int 		players_on_floor[MAX_PLAYERS];
	pos_t		xPos[MAX_PLAYERS];
	pos_t		yPos[MAX_PLAYERS];
	vel_t		xVel[MAX_PLAYERS];
	vel_t		yVel[MAX_PLAYERS];
} PKT_ALL_POS_UPDATE;

typedef struct pkt12{
	playerNo_t 	player_number;
	floorNo_t 	current_floor;
	floorNo_t 	desired_floor;
} PKT_FLOOR_MOVE_REQUEST;

typedef struct pkt13{
	floorNo_t 	new_floor;
	pos_t		xPos;
	pos_t		yPos;
} PKT_FLOOR_MOVE;

#endif
