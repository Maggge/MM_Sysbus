/*
    MM_Sysbus STM32-Can
    Copyright (C) 2021  Markus Mair, https://github.com/Maggge/MM_Sysbus

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

#ifndef __MM_CAN_STM32__
#define __MM_CAN_STM32__

#include <Arduino.h>
#include <MM_Sysbus.h>
#include <eXoCAN.h>
#include "MM_CAN.h"

class MM_STM32_CAN : public MM_Interface {
private:

    /**
     * CAN Bus Speed
     */
    BitRate _bitRate;

    /**
     * CAN Crystal Frequency
     */
    BusType _busType;
public:
    /**
     * CAN-Bus object
     * @see STM_CAN
     * @see 
     */
    eXoCAN _can;


       /**
     * Constructor for STM32 Can
     * @param bitRate bitRate of CAN-Bus BR125K for 125 kbit
     * @param busType CAN bus type PORTA_11_12_WIRE_PULLUP
     * @see https://github.com/Maggge/eXoCAN/eXoCAN.h
     */
    MM_STM32_CAN(BitRate bitRate, BusType busType);

    bool begin(void);

    /**
     * Send message to CAN-bus
     * @param type 2 bit message type (MM_PKGTYPE_*)
     * @param target address between 0x0001 and 0x07FF/0xFFFF
     * @param source source address between 0x0001 and 0x07FF
     * @param port port address between 0x00 and 0x1F, Unicast only
     * @param len number of bytes to send (0-8)
     * @param data array of bytes to send
     */
    bool Send(MM_MsgType msgType, uint16_t target, uint16_t source, uint8_t port, uint8_t len, uint8_t *data);

    /**
     * Receive a message from the CAN-bus
     *
     * This polls the CAN-Controller for new messages.
     * The received message will be passed to &pkg, if no message
     * is available the function will return false.
     *
     * @param pkg MMPacket-Reference to store received packet
     * @return true if a message was received
     */
    bool Receive(MM_Packet &pkg);
};



#endif