/*
    MM_Sysbus - DIY System-Bus

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

#ifndef __MM_Sysbus__
#define __MM_Sysbus__

//#define MM_DEBUG 


//Max number of bus-interfaces
#ifndef MM_BUSNUM
    #define MM_BUSNUM 3
#endif

//Max number of attached hooks
#ifndef MM_HOOKNUM
    #define MM_HOOKNUM 5
#endif

//Max number of modules
#ifndef MM_MODULNUM
    #define MM_MODULNUM 5
#endif

#include <Arduino.h>
#include <EEPROM.h>
#include <avr/wdt.h>

#include "MM_Protocol.h"

#include "MM_Interface.h"
#include "MM_UART.h"
#include "MM_CAN.h"

#include "MM_Hook.h"
#include "MM_Module.h"




//MM_Sysbus Controller
class MM_Sysbus{
private:
    /**
    * Node ID of this controller
    */
    uint16_t _nodeID = 0;

    /**
     * Indicates if the controller (and also the modules) use EEPROM as config-storage 
     */
    bool _useEEPROM = false;
    
    /**
     * First EEPROM address to use
     */
    uint16_t _EEPROMaddr = 0;

    /**
     * this variable is set from the constructor and tells the firstboot() function if is the first boot;
     * If the controller is defined without eeprom firstboot is called every boot
     */
    bool _firstboot;

    /**
     * Pin number of status LED
     */
    uint8_t _statusLED;

    /**
     * Pin number of config Button
     */
    uint8_t _cfgBtn;


    /**
     * Attached communication interfaces
     */
    MM_Interface *_interfaces[MM_BUSNUM];

    /**
     * Attached hooks
     */
    MM_Hook _hooks[MM_HOOKNUM];

    /**
     * Attached Modules
     */
    MM_Module *_modules[MM_MODULNUM];

    /**
     * Indicates if the controller has a nodeId
     */
    bool _initialized;

    /**
     * Initialization Mode
     * For set the nodeID or reset the node
     */
    void initialization();

    /**
     * Indicates if the cfg Button is pressed or not
     */
    volatile bool _cfgBtnPressed = false;

    /**
     * stores the last time(millis()) the cfg Button triggered
     */
    volatile uint32_t _cfgBtnTriggerTime = 0;

    /**
     * reads if the cfg Button is pressed and set _cfgBtnPressed and
     * store the time(millis()) when the cfg Button triggerd in _cfgBtnPressTime
     */
    void processCfgButton();

public:
    /**
     * Controller with fixed boot node-Id and without eeprom config-storage.
     * @param nodeID between 1 and 2047
     */
    MM_Sysbus(uint16_t nodeID);

    /**
     * Controller with fixed node-Id on first boot and eeprom config-storage.
     * @param nodeID between 1 and 2047
     * @param EEPROMstart start Address of EEPROM config-storage (usually 0), if you need eeprom for other Applications you can increase it.
     * Attention! If @param EEPROMstart is too high crashes or data losses are possible!
     */
    MM_Sysbus(uint16_t nodeID, uint16_t EEPROMstart);

    /**
     * Controller with eeprom config-storage, an config button and an status Led for node addressing and configuring over bus.
     * @param cfgButton - config button pin.
     * @param statusLED - status led pin
     * @param EEPROMstart start Address of EEPROM config-storage (usually 0), if you need eeprom for other Applications you can increase it.
     * Attention! If @param EEPROMstart is too high crashes or data losses are possible!
     */
    MM_Sysbus(uint8_t cfgButton, uint8_t statusLED, uint16_t EEPROMstart);

    /**
    * Function for initial config on the first boot or every boot if eeprom isn't used
    * @param function This function is called if the node boots the first time or is configured whitout eeprom
    */
    bool firstboot(void (*function)());

    /**
     * set the nodeID
     * @param nodeID between 1 and 2047
     */
    bool setNodeId(uint16_t nodeID);

    /**
     * @return returns the node-Id of the controller
     */
    uint16_t nodeID();

    /**
     * Attach a bus-interface to the controller
     * @param bus Bus object, derived from MM_Interface
     * @return true if successful, false on error
     */
    bool attachBus(MM_Interface* bus);

    /**
     * Detach the given Bus object
     * @param bus Bus object, derived from MM_Interface
     * @return true if successful, false if the bus isn't attached
     */
    bool detachBus(MM_Interface* bus);

    /**
     * Send a message to all attached buses
     * @param pkg complete MM_Packet to send
     */
    bool Send(MM_Packet pkg);

    /**
     * Send a message to all attached buses
     * @param meta message meta
     * @param len length of the data
     * @param data data to send
     * @return true if successful, false if errors occurred
     */
    bool Send(MM_Meta meta, uint8_t len, uint8_t *data);

    /**
     * Send a message to all attached buses
     * @param msgType MM_MsgType
     * @param target target address between 0 and 2047(Unicast)/65535(Multicast)
     * @param len length of the data
     * @param data data to send
     * @return true if successful, false if errors occurred
     */
    bool Send(MM_MsgType msgType, uint16_t target, uint8_t len, uint8_t *data);

    /**
     * Send a message to all attached buses
     * @param msgType MM_MsgType
     * @param target target address between 0 and 2047(Unicast)/65535(Multicast)
     * @param port port address between 0 and 31, Unicast(target port) and Broadcast(sender port)
     * @param len length of the data
     * @param data data to send
     * @return true if successful, false if errors occurred
     */
    bool Send(MM_MsgType msgType, uint16_t target, uint8_t port, uint8_t len, uint8_t *data);

    /**
     * Send a message to all attached buses
     * @param msgType MM_MsgType
     * @param target target address between 0 and 2047(Unicast)/65535(Multicast)
     * @param source source address between 0 and 2047, 0=self
     * @param port port address between 0 and 31, Unicast(target port) and Broadcast(sender port)
     * @param len length of the data
     * @param data data to send
     * @param skipInterface busID to skip when sending (on routing we skip the interface the message originated from). -1 if no interface should skipped.
     * @return true if successful, false if errors occurred
     */
    bool Send(MM_MsgType msgType, uint16_t target, uint16_t source, uint8_t port, uint8_t len, uint8_t *data, signed char skipInterface);

    /**
     * Receives a single message from the attached Interfaces
     * The message from the interface with the lowest id that has a new message is stored in pkg
     * The pkg received will be redistributed over all attached interfaces
     * @param pkg reference to store the received MM_Packet
     * @return true - a message was received, false - no new message 
     */
    bool Receive(MM_Packet &pkg);

    /**
     * Receives a single message from the attached Interfaces
     * The message from the interface with the lowest id that has a new message is stored in pkg
     * @param pkg reference to store the received MM_Packet
     * @param routing if false the pkg will not be redistributed over all attached interfaces
     * @return true - a message was received, false - no new message
     */
    bool Receive(MM_Packet &pkg, bool routing);

    /**
     * Process incoming packet
     * @param pkg received Packet
     */
    void Process(MM_Packet &pkg);

    /**
     * Attach a module to this controller
     *
     * @param cfgId is only needed if you want restore the EEPROM config
     * @param MM_Module MM_Module to attach
     */
    bool attachModule(MM_Module *module, uint8_t cfgId);

    /**
     * Attach a module to this controller
     *
     * @param MM_Module MM_Module to attach
     * @return cfgId in EEPROM, 255 if not successfull
     */
    uint8_t attachModule(MM_Module *module);

    /**
     * Detach a module from this controller
     *
     * @param module to detach
     * @return true if successfully detached, false if module not found
     */
    bool detachModule(MM_Module *module);

    /**
     * Attach a hook to a set of metadata
     *
     * If a received packet matches the type, target and port the supplied function
     * will be called. This allows for custom actors.
     *
     * @param type 2 bit message type (MM_PKGTYPE)
     * @param target target address between 0x0001 and 0x07FF/0xFFFF, 0x0 = everything
     * @param port port address between 0x00 and 0x1F, Unicast only, -1 = everything
     * @param First data byte (usually MM_CMD), ALL_CMD = everything
     * @param function to call when matched
     * @return true if successfully added
     */
    bool hookAttach(MM_MsgType msgType, uint16_t target, uint8_t port, MM_CMD cmd, void (*function)(MM_Packet&));

    /**
     * Main loop
     * Receives and routes packets, loop attached modules, etc
     * @return Packet last received packet
     */
    MM_Packet loop();

    /**
     *  returns the EEPROM Address for the register of the module
     *  @param cfgID the Id of the Module
     */

    uint16_t getEEPROMAddress(uint8_t cfgID);
    
    /**
     * resets the whole controller - clears the eeprom, reboot and initialize the controller and modules
     */
    void reset();

};

#endif




