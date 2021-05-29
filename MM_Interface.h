/*
    MM_Sysbus Interface

    Copyright (C) 2020  Markus Mair

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef __MM_Interface__
#define __MM_Interface__

#include "MM_protocol.h"

/**
 * Base class for any communication interface
 * This is only a template
 */
class MM_Interface
{
public:
    /**
     * Error code of the last action - 0 = no error
     */
    uint8_t lastErr = 0;

    /**
     * Initialize Interface
     * @return true if all ok
     */
    virtual bool begin(void) = 0;

    /**
     * Send message to the interface
     * @param type MM_MsgType
     * @param target target address between 0 and 2047(Unicast)/65535(Multicast)
     * @param source source address between 0 and 2047
     * @param port port address between 0 and 31, Unicast only
     * @param len number of bytes to send (0-8)
     * @param data array of bytes to send
     * @return true if send was successful, false if errors occured and store the error in lastErr
     */
    virtual bool Send(MM_MsgType type, uint16_t target, uint16_t source, uint8_t port, uint8_t len, uint8_t *data) = 0;

    /**
     * Receive a message from the interface
     * @param pkg reference to store received packet
     * @return true if a message was received
     */
    virtual bool Receive(MM_Packet &pkg)=0;
};

#endif