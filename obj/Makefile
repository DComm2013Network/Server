#MakeFile

CC=gcc
# Hey!, I am comment number 2. I want to say that CFLAGS will be the
# options I'll pass to the compiler.
CFLAGS=-c -Wall


all: Server

Server: Startup.o ConnectionManager.o GameplayController.o GeneralController.o InboundSwitchboard.o OutboundSwitchboard.o UI.o
	$(CC) Startup.o ConnectionManager.o GameplayController.o GeneralController.o InboundSwitchboard.o OutboundSwitchboard.o UI.o -o ../bin/Server

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
