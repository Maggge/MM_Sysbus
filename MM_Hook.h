/*
    MM_Sysbus Hook

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

#ifndef __MM_HOOK__
#define __MM_HOOK__

#include "MM_Protocol.h"

/**
 * Hook struct
 * Contains address and function to call on RX
 */
struct MM_Hook{
    /**
     * Message type to listen
     */
    MM_MsgType type = Multicast;

    /**
     * Port
     * 0 - 31
     * Only used in Unicast Mode
     * -1 -> Everything
     */
    char port = -1;

    /**
     * Target address
     * Unicast:             1 - 2047
     * Multicast:           1 - 65535
     * 0 -> everything
     */
    uint16_t target = 0;

    /**
     * Command to listen
     * ALL_CMDS = everything
     */
    MM_CMD cmd;

    /**
     * Function to call
     */
    void (*execute)(MM_Packet &pkg);

};

#endif