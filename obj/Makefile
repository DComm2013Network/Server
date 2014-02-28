#MakeFile

CC=gcc
CFLAGS=-c -Wall
GBD=-g
LIBS=-lpthread

all: Server

Server: Sockets.o Startup.o ConnectionManager.o GameplayController.o GeneralController.o InboundSwitchboard.o OutboundSwitchboard.o UI.o
	$(CC) $(GDB) Sockets.o Startup.o ConnectionManager.o GameplayController.o GeneralController.o InboundSwitchboard.o OutboundSwitchboard.o UI.o -o ../bin/Server $(LIBS)

Sockets.o: ../src/Sockets.c
	$(CC) $(CFLAGS) ../src/Sockets.c

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

clean:
	rm -rf *o ../bin/Server
