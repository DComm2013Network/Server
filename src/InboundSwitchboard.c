/*
 * InboundSwitchboard.c
 *
 *  Created on: 13 Feb 2014
 *      Author: chris
 */


/*
 * INBOUND SWITCHBOARD
    allocate socket lists
    while 1
        listen on all sockets
        if new connection
            add new connection to socket to list of inbound sockets
            pass socket+info to outbound switchboard

        if movement packet
            pass to gameplay controller

        if game status packet
            pass to game status controller

            .... etc
 */
