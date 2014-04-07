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
#define MAX_FLOORS		9
#define MAX_NAME	 	15
#define MAX_MESSAGE		180
#define MAX_OBJECTIVES	32

#define NUM_NET_PACKETS 16


// Other Includes
#include <time.h>
#include <stdint.h>

// Definitions for various game data types
typedef uint32_t    floorNo_t;
typedef uint32_t    playerNo_t;
typedef uint32_t    teamNo_t;
typedef uint32_t    status_t;
typedef uint32_t    pos_t;
typedef uint32_t    character_t;
typedef float	    vel_t;
typedef uint32_t    packet_t;
typedef uint64_t    sequence_t;
typedef uint32_t    bool_t;
typedef uint32_t    tile_t;

// Connect code Definitions
#define connectCode_ACCEPTED	0x001
#define connectCode_DENIED		0x000

// Game Status Definitions
#define GAME_STATE_WAITING		0x001   // Waiting for PLAYER_STATE_READY by all players (LOBBY)
#define GAME_STATE_ACTIVE		0x002   // Game engine running
#define GAME_STATE_OVER			0x003

#define GAME_TEAM1_WIN			0x006
#define GAME_TEAM2_WIN			0x007

// Player Status Definitions
#define PLAYER_STATE_WAITING	0x005   // Player just joined in lobby
#define PLAYER_STATE_READY		0x004   // Ready to join game - player chosen team
#define PLAYER_STATE_DROPPED    0x008   // Joined and disconnected
#define PLAYER_STATE_OUT        0x009   // Tagged sent to lobby
#define PLAYER_STATE_INVALID    0x010   // Not a player - used when MAX_PLAYERS > number of players allowed to join a game
#define PLAYER_STATE_ACTIVE     0x011   // Player is in the game world running around
#define PLAYER_STATE_AVAILABLE  0x012   // Available slot for player to join - value cannot be defined to 1 or 2!!

// Objective statuses
#define OBJECTIVE_INVALID       0x000
#define OBJECTIVE_AVAILABLE     0x001
#define OBJECTIVE_CAPTURED      0x002

// Special floor Definitions
#define FLOOR_LOBBY				0x000

// Team Definitions
#define TEAM_NONE       0
#define TEAM_COPS       1
#define TEAM_ROBBERS    2

#ifndef PACKETS
#define PACKETS
// Packet Definitions

typedef struct pkt01{
	char 		playerName[MAX_NAME];
	character_t selectedChatacter;
} PKT_PLAYER_JOIN;

typedef struct pkt02{
	status_t 	connectCode;
	playerNo_t 	clients_playerNumber;
	teamNo_t 	clients_team_number;
	char        playerName[MAX_NAME];
} PKT_JOIN_RESPONSE;

typedef struct pkt03{
	bool_t 	    playerValid[MAX_PLAYERS];
	char 		playerNames[MAX_PLAYERS][MAX_NAME];
	teamNo_t 	playerTeams[MAX_PLAYERS];
    character_t characters[MAX_PLAYERS];
	status_t	readystatus[MAX_PLAYERS];
} PKT_PLAYERS_UPDATE;

typedef struct pkt04{
	playerNo_t 	sendingPlayer;
	char 		message[MAX_MESSAGE];
} PKT_CHAT;

typedef struct pkt05{
	playerNo_t	playerNumber;
	status_t	ready_status;
	teamNo_t	team_number;
	char 		playerName[MAX_NAME];
} PKT_READY_STATUS;

typedef struct pkt06{
    playerNo_t  placingPlayer;
    floorNo_t   floor;
    pos_t       xPos;
    pos_t       yPos;
    tile_t      tile;
} PKT_SPECIAL_TILE;

typedef struct pkt07{
    packet_t    type;
    //	<< UNPURPOSED >>
} PKT_7;

typedef struct pkt08{
	status_t	objectiveStates[MAX_OBJECTIVES];
	status_t	game_status;
} PKT_GAME_STATUS;

#define KEEP_ALIVE              0x9

typedef struct pkt10{
	floorNo_t 	floor;
	playerNo_t 	playerNumber;
	pos_t 		xPos;
	pos_t		yPos;
	vel_t		xVel;
	vel_t		yVel;
} PKT_POS_UPDATE;

typedef struct pkt11{
	floorNo_t 	floor;
	bool_t	    playersOnFloor[MAX_PLAYERS];
	pos_t		xPos[MAX_PLAYERS];
	pos_t		yPos[MAX_PLAYERS];
	vel_t		xVel[MAX_PLAYERS];
	vel_t		yVel[MAX_PLAYERS];
} PKT_ALL_POS_UPDATE;

typedef struct pkt12{
	playerNo_t 	playerNumber;
	floorNo_t 	current_floor;
	floorNo_t 	desired_floor;
	pos_t       desired_xPos;
	pos_t       desired_yPos;
} PKT_FLOOR_MOVE_REQUEST;

typedef struct pkt13{
	floorNo_t 	newFloor;
	pos_t		xPos;
	pos_t		yPos;
} PKT_FLOOR_MOVE;

typedef struct pkt14 {
    playerNo_t  tagger_id; /* the person who tagged */
    playerNo_t  taggee_id; /* the person who got tagged */
} PKT_TAGGING;

typedef struct pkt15 {
    uint32_t    data;
    uint16_t     vel;
} PKT_MIN_POS_UPDATE;
#define MIN_10 15

typedef struct pkt16 {
    uint8_t     floor;
    uint32_t    playersOnFloor;
    uint32_t    xPos[11];
    uint32_t    yPos[11];
    uint16_t     vel[32];
} PKT_MIN_ALL_POS_UPDATE;
#define MIN_11 16

#endif
