/*
    MM_Sysbus MCP-Can

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

#ifndef __MM_CAN_SPI__
#define __MM_CAN_SPI__

#include <Arduino.h>
#include <MM_Sysbus.h>
#include <mcp_can.h>
#include <SPI.h>
#include "MM_Module.h"


/**
 * Global variable indicating a CAN-bus got a message
 * This is currently shared amongst all CAN-interfaces!
 */
extern volatile bool MM_CANIntReq;

/**
 * Global interrupt function called on CAN interrupts
 * This is currently shared amongst all CAN-interfaces!
 */
void MM_CANInt(void);

/**
 * CAN Communication Interface
 * @see MM_Interface
 */

class MM_CAN : public MM_Interface {
private:
    

    uint8_t _csPin = 0;

    /**
     * Interrupt pin to be used
     */
    uint8_t _intPin = 0;

    /**
     * CAN Bus Speed
     */
    uint8_t _speed = 0;

    /**
     * CAN Crystal Frequency
     */
    uint8_t _clockspd = 0;
public:
    /**
     * CAN-Bus object
     * @see MCP_CAN
     * @see https://github.com/Seeed-Studio/CAN_BUS_Shield
     */
    MCP_CAN _interface;



    /**
     * Constructor for Interrupt based operation
     * @param cs pin used for CHIP_SELECT
     * @param speed CAN bus speed definition from mcp_can_dfs.h (bottom)
     * @param clockspd MCP crystal frequency from mcp_can_dfs.h (bootom)
     * @param interrupt pin used for interrupt. Must have hardware INT support
     * @see https://github.com/coryjfowler/MCP_CAN_lib.git
     */
    MM_CAN(uint8_t cs, uint8_t speed, uint8_t clockspd, uint8_t interrupt);

    /**
     * Initialize Interface
     * @return true if all ok
     */
    bool begin(void);

    /**
     * Parse CAN-address into our metadata format
     * @param canAddr CAN-address
     * @return MM_Meta object containing decoded metadata, targst/source==0x00 on errors
     */
    static MM_Meta CanAddrParse(uint32_t canAddr);

    /**
     * Assemble a CAN-address based on our adressing format
     * @param meta MM_Meta object
     * @return unsigned long CAN-address
     */
    static uint32_t CanAddrAssemble(MM_Meta meta);

    /**
     * Assemble a CAN-address based on our adressing format
     * @param type 2 bit message type (MM_MsgType)
     * @param target address between 0x0001 and 0x07FF/0xFFFF
     * @param source source address between 0x0001 and 0x07FF
     * @return unsigned long CAN-address
     */
    static uint32_t CanAddrAssemble(MM_MsgType msgType, uint16_t target, uint16_t source);

    /**
     * Assemble a CAN-address based on our adressing format
     * @param type 2 bit message type (MM_MsgType)
     * @param target address between 0x0001 and 0x07FF/0xFFFF
     * @param source source address between 0x0001 and 0x07FF
     * @param port port address between 0x00 and 0x1F, Unicast only
     * @return unsigned long CAN-address
     */
    static uint32_t CanAddrAssemble(MM_MsgType msgType, uint16_t target, uint16_t source, uint8_t port);

    /**
     * Send message to CAN-bus
     * @param type 2 bit message type (MM_MsgType)
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