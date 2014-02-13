/*---------------------------------------------------------------------* 
-- HEADER FILE: Server.h 		The definitions and declarations to be
-- 								to be used to communicate between the
-- 								components of the Server
--
-- PROGRAM:		Game-Server
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
-- This is unique to the server
----------------------------------------------------------------------*/

#define SOCKET int

//function prototypes
int UI(SOCKET uiSockSet);
int ConnectionManager(SOCKET connectionSockSet, SOCKET outswitchSockSet);
int GameplayController(SOCKET gameplaySockSet, SOCKET outswitchSockSet);
int OutboundSwitchboard(SOCKET outswitchSockSet);
int InboundSwitchboard(SOCKET connectionSockSet, SOCKET generalSockSet, SOCKET gameplaySockSet, SOCKET outswitchSockSet);
int GeneralController(SOCKET generalSockSet,SOCKET outswitchSockSet);
