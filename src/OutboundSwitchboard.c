/*
 * OutboundSwitchboard.h
 *
 *  Created on: 13 Feb 2014
 *      Author: chris
 */

/*
 * OUTBOUND SWITCHBOARD
    allocate tcp and udp socket list
    while 1
        listen on socket
        if new connection
            add tcp socket descriptor
            open udp socket
        else
            forward to esired players
 */
