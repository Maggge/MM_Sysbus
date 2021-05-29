/*
    MM_Sysbus Protocol
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

#ifndef __MM_Protocol__
#define __MM_Protocol__

enum MM_MsgType{
    Unicast,
    Multicast,
    Broadcast,
    Streaming,
};

enum MM_CMD{
    RESET_NODE  = 0x01, //Factory Reset
    NODE_ID     = 0x02, //Change the nodeID
    NODE_BOOT   = 0x03, //This is sent by controller after booting
    NODE_PING   = 0x04, //Sent to check if a node is alive
    NODE_PONG   = 0x05, //Automatic answer to ping
    MOD_TYPE    = 0x06, //Module Type - 2byte , modulType, _useEEPROM  
    REQ_PRES    = 0x07, //Request the Presentation Message from each Module 
    REQ         = 0x08, //Request the state of a value - 1-byte - type of value(MM_CMD)
    ERROR       = 0x09, //The last Message received can't processed due to an error
    REG_SET     = 0x0A, //Set registers 1st byte Register-index and 1-6 bytes register data
    REG_GET     = 0x0B, //Request registers 1st byte Register-index, 2nd byte how much registers(if you request more then 6 the message will be splittet!)
    REG_COMMIT  = 0x0C, //Commits the set or requestet Registers 1st byte Register-index and 1-6 bytes register data
    GROUP_ADD   = 0x0D, //Add a Multicast address, 2-byte-address
    GROUP_REM   = 0x0E, //Remove a Multicast address, 2-byte-address
    GROUP_CLEAR = 0x0F, //Remove all Multicast addresses
    
    
    BOOL        = 0x51, //1-Bit, on/off

    DIM_UP      = 0x90, //Dim up, 1-Byte - amount(percent)
    DIM_DOWN    = 0x91, //Dim down, 1-Byte - amount(percent)
    SET_SCENE   = 0x92, //1 Byte, choose active scene by id
    NEXT_SCENE  = 0x93, //0-Bit, change scene to next id
    PREV_SCENE  = 0x94, //0-bit, change scene to prev id
    DATE        = 0x97, //4-byte, century, year, month, day 
    TIME        = 0x98, //3-byte, hour, minute, second
    DATE_TIME   = 0x99, //combine DATE and TIME - 7-byte - century, year, month, day, hour, minute, second


    CONTROL_MODE= 0x9D, //
    SET_TEMP    = 0x9E, //
    HYS         = 0x9F, //
    TEMP        = 0xA0, //Temperature, x*0.1°C, int
    HUM         = 0xA1, //Humidity, x*0.1%RH, unsigned int
    PRS         = 0xA2, //Pressure, x*0.1hPa, unsigned int
    LUX         = 0xA5, //unsigned long
    UV          = 0xA6, //*0.1, unsigned int
    IR          = 0xA7, //unsigned long
    PM25        = 0xB0, //TBD
    PM10        = 0xB1, //TBD
    VOLT        = 0xC0, //x*0.01V, int
    AMP         = 0xC1, //x*0.01A, int
    PWR         = 0xC2, //x*0.1W or VA, long
    PER         = 0xD0, //%, byte
    PML         = 0xD1, //‰, unsingned int
    PPM         = 0xD2, //Parts per million, unsingned int
    SPYEAR      = 0xD5, //smth per Year, unsinged int
    SPMONTH     = 0xD6, //smth per Month, unsinged int
    SPDAY       = 0xD7, //smth per Day, unsinged int
    SPHOUR      = 0xD8, //smth per Hour, unsinged int
    SPMINUTE    = 0xD9, //smth per Minute, unsinged int (RPM, Pulse PM, etc)
    SPSECOND    = 0xDA, //smth per Second, unsinged int
    SAT         = 0xE2, //Saturation 0 - 255
    BRI         = 0xE3, //Brightness 0 - 255
    RGB         = 0xE4, //RGB
    RGBW        = 0xE5, //RGBW
    RGBWW       = 0xE6, //RGBWW
    HSV         = 0xE7, //4-byte, uint16_t hue(0-360), uint8_t sat(0-255),uint8_t val(0-255)
    SCENE       = 0xE8, //Activate Scene 1 byte SceneID;
    PWM         = 0xE9, //1-byte pwm value between 0 and 255
    KWH         = 0xEA, //Kwh, float value (4 bytes)
    KWH_TODAY   = 0xEB, //Kwh generated or consumed today, float value (4 bytes)
    KWH_WEEK    = 0xEC, //Kwh generated or consumed this week, float value (4 bytes)
    KWH_MONTH   = 0xED, //Kwh generated or consumed this month, float value (4 bytes)
    KWH_YEAR    = 0xEE, //Kwh generated or consumed this year, float value (4 bytes)
    GR          = 0xF0, //Weight in gramm, float value (4 bytes)
    KG          = 0xF1, //Weight in kilogramm, float value (4 bytes)
    TON         = 0xF2, //Weight in tonns, float value (4 bytes)
    STREAM_START= 0xFC, //Start a Stream of type, MM_Stream (1 byte)
    STREAM_END  = 0xFE, //Terminate the stream
    ALL_CMDS    = 0xFF,  //For hook filter only to execute on every cmd
};

/**
 * 
 */
enum MM_Stream {

};

/**
 * Msg meta
 * Contains msg-type, source and target addresses (and port) and the interface id the message originated from   
 */
struct MM_Meta {
    /**
     * Message type
     *  0 -> Broadcast
     *  1 -> Unicast
     *  2 -> Multicast
     */
    MM_MsgType type = Broadcast;

    /**
     * Port
     * 0 - 31
     * Used in Unicast and Broadcast Mode, otherwise 0
     */
    uint8_t port = 0;

    /**
     * Target address
     * Broadcast:           0
     * Unicast:             1 - 2047
     * Multicast:           1 - 65535
     */
    uint16_t target = 0;

    /**
     * Source address
     * 1 - 2047
     * 0 = packet from Master
     */
    uint16_t source = 0;

    /**
     * Interface-id the message originated from
     * -1: Message is generated on this node
     */
    signed char busId = -1;
};

/**
 * Packet
 * Contains the meta, the length and the data
 */
struct MM_Packet{
    /**
     * MM_Meta
     */
    MM_Meta meta;

    /**
     * length: 0-8 bytes
     * -1 indicates invalid packet
     */
    char len = -1;

    /**
     * msg data
     */
    uint8_t data[8];
};

enum MM_ModuleType{

};

#endif