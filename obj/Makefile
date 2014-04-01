#MakeFile

CC=gcc
CFLAGS=-c -Wall
GBD=-g
LIBS=-lpthread -lm
OBJECTS=server-utils.o Startup.o ConnectionManager.o GameplayController.o GeneralController.o InboundSwitchboard.o OutboundSwitchboard.o UI.o keep-alive.o game-utils.o chat.o min-packet-utils.o move-timer.o

all: Server

Server: $(OBJECTS)
	$(CC) $(GDB) $(OBJECTS) -o ../bin/Server $(LIBS)

server-utils.o: ../src/server-utils.c
	$(CC) $(CFLAGS) ../src/server-utils.c

Startup.o: ../src/Startup.c
	$(CC) $(CFLAGS) ../src/Startup.c 

ConnectionManager.o: ../src/ConnectionManager.c
	$(CC) $(CFLAGS) ../src/ConnectionManager.c 

GameplayController.o: ../src/GameplayController.c
	$(CC) $(CFLAGS) ../src/GameplayController.c

GeneralController.o: ../src/GeneralController.c
	$(CC) $(CFLAGS) ../src/GeneralController.c
	
InboundSwitchboard.o: ../src/InboundSwitchboard.c
	$(CC) $(CFLAGS) ../src/InboundSwitchboard.c
	
OutboundSwitchboard.o: ../src/OutboundSwitchboard.c
	$(CC) $(CFLAGS) ../src/OutboundSwitchboard.c
	
UI.o: ../src/UI.c
	$(CC) $(CFLAGS) ../src/UI.c

keep-alive.o: ../src/keep-alive.c
	$(CC) $(CFLAGS) ../src/keep-alive.c

game-utils.o: ../src/game-utils.c
	$(CC) $(CFLAGS) ../src/game-utils.c

chat.o: ../src/chat.c
	$(CC) $(CFLAGS) ../src/chat.c

min-packet-utils.o: ../src/min-packet-utils.c
	$(CC) $(CFLAGS) ../src/min-packet-utils.c

move-timer.o: ../src/move-timer.c
	$(CC) $(CFLAGS) ../src/move-timer.c

clean:
	rm -rf *o ../bin/Server
