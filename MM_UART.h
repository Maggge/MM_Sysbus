/*
    MM_Sysbus Serial-Interface

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
#ifndef __MM_UART__
#define __MM_UART__

#include <Arduino.h>
#include <MM_Sysbus.h>
#include "Stream.h"

/**
 * UART Communication Interface
 * @see MM_COMM
 */
class MM_UART : public MM_Interface {
    private:
        /**
         * UART reference pointer
         */
        //unsigned int _interface;
        Stream *_interface;

        /**
         * Incoming data buffer
         */
        uint8_t _buf[35];

        /**
         * Search for next start byte and shift buffer
         * @return byte start byte found
         */
        bool bufShift();

        /**
         * Remove len bytes from the buffer
         * @return byte start byte found
         */
        bool bufShift(byte len);

    public:
        /**
         * Constructor for UART interface
         * @param serial object used for this communication
         */
        MM_UART(Stream &serial);

        /**
         * Initialize UART, just a dummy
         * @return byte error code, always 0/success
         */
        bool begin(void);

        /**
         * Send message to UART-bus
         * @param type 2 bit message type (MM_PKGTYPE_*)
         * @param target address between 0x0001 and 0x07FF/0xFFFF
         * @param source source address between 0x0001 and 0x07FF
         * @param port port address between 0x00 and 0x1F, Unicast only
         * @param len number of bytes to send (0-8)
         * @param data array of bytes to send
         */
        bool Send(MM_MsgType msgType, uint16_t target, uint16_t source, uint8_t port, uint8_t len, uint8_t *data);

        /**
         * Receive a message from the UART-bus
         *
         * This polls the UART-Controller for new messages.
         * The received message will be passed to &pkg, if no message
         * is available the function will return false.
         *
         * @param pkg Packet-Reference to store received packet
         * @return true if a message was received
         */
        bool Receive(MM_Packet &pkg);

        /**
         * Convert ASCII hex to byte
         *
         * This will convert the characters 0-9, A-F and a-f to an
         * byte-value between 0 and 15. Other inputs will return 0
         *
         * @param byte single ASCII hex character
         * @return corresponding numeric byte value
         */
        uint8_t HexToByte(uint8_t hex);
};

#endif