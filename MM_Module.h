/*
    MM_Sysbus Module

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

#ifndef __MM_Module__
#define __MM_Module__

#include <Arduino.h>
#include <EEPROM.h>

#include "MM_Protocol.h"

/**
 * Because MM_Sysbus has not been defined yet
 */
class MM_Sysbus;

#ifndef MULTICAST_TARGETS
    #define MULTICAST_TARGETS 5
#endif

#ifndef REGISTER_PER_MODUL
    #define REGISTER_PER_MODUL 64
#endif

/**
 * Register Class
 * Load/Store and manage the EEPROM-Registers
 * The first Register is reserved for the cfgID and must be set before loading otherwise loading fails!
 */
class Register{
    private:
        /**
         * Pointer to register Variables
         */
        uint8_t *_regs[REGISTER_PER_MODUL];

        /**
         * Number of registers occupied
         */
        uint8_t _regsCount = 0;

    public:

        /**
         * EEPROM-Start-Address of the Register
         * This Address we request from the Controller
         */
        uint16_t eepromAddress = 65535;

        template<typename T> 
        bool addRegister(const T *reg);

        bool setRegister(uint8_t index, uint8_t len, uint8_t* data);
        uint8_t getRegister(uint8_t index);

        bool saveRegister();
        bool loadRegister();

        bool clearRegister();
};


/**
 * Multicast Targets class
 * Holds and handle Multicast Targets
 * To store the targets in the register call 
 */
class MulticastTargets{
    private:

        Register *_reg;
    
    public:
        /**
         * Target Addresses for Multicast
         */
        uint16_t _targets[MULTICAST_TARGETS];

        /**
         * Add register to store the target list
         * @param reg pointer of the Register to store the list
         */
        void useRegister(Register *reg);

        /**
         * Add a Target to the list
         * @param target address to add
         * @return true if successful, false if target is already in list or list is full
         */
        bool addTarget(uint16_t target);

        /**
         * Remove a Target from the list
         * @param target address to remove
         * @return true if successful, false if target isn't in list
         */
        bool removeTarget(uint16_t target);

        /**
         * Checks if a Target is in the list
         * @param target address to check
         * @return true if Target is in list
         */
        bool checkTarget(uint16_t target);

        /**
         * Clears the whole list
         * @return true if successful
         */
        bool clearTargets();
};

/**
 * Base class for modules
 * This is a template to implement modules
 * It handles the config storage an some base functions
 */
class MM_Module{
    public:
        /**
         * Pointer to our Controller
         */
        MM_Sysbus *_controller;

    protected:

        /**
         * Type of the module
         * The module Type is submitted in the MOD_Type Message at startup and
         * to check if the config in EEPROM is for this module
         */
        MM_ModuleType _moduleType;

        /**
         * ID of the module
         * gets set by MM_Sysbus on attach Module to get EEPROM_address etc. 
         */
        uint8_t _cfgId = 255;
        
        /**
         * indicates if we use EEPROM
         */
        bool _useEEPROM = false;

        /**
         * Register holds the whole config
         * _register[0] is reserved for moduleID
         * If we use EEPROM we must call addRegister(_moduleType) at first in the begin() otherwise the cfg will be corrupted!
         * 
         */
        Register _register;

        /**
         * _multicastTargets
         * 
         */
        MulticastTargets _multicastTargets;

        /**
         * Target Port for Unicast
         */ 
        uint8_t _port = 0;

        /**
         * Check if the Message is for this module(Unicast and Multicast)
         * if the message is a cfgMessage and this function can do the work returns false;
         * @param pkg the received Package
         * @return return true if the package is for the module
         */
        bool checkMsg(MM_Packet &pkg);

        /**
         * Reset current configuration
         * @return bool true if successful
         */
        bool cfgReset();

    public:

        /**
         * Configure pins etc.
         * @return bool true if successful
         */
        virtual void begin(bool useEEPROM, uint8_t cfgId);

        /**
         * Process incoming packet
         * @param pkg Pack
         * @return bool true if successful
         */
        virtual bool process(MM_Packet &pkg);

        /**
         * Main loop
         * @return bool true if successful
         */
        virtual bool loop();

        /**
         * Broadcast the state/states of the module
         * @return return true if successful
         */
        virtual bool broadcastState();

        /**
         * Broadcast the Module Type if _controller != NULL 
         */
        void broadcastModuleType();
};

#endif