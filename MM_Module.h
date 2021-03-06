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
    #define MULTICAST_TARGETS 10
#endif

#ifndef MAX_CONFIG_SIZE
    #define MAX_CONFIG_SIZE 64
#endif

/**
 * Target-struct
 */
struct MM_Target
{
    uint16_t address;
    MM_CMD filter;
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

        /**
         * return the port of the module
         */
        uint8_t port();

        /**
         * return the cfgid of the module
         */
        uint8_t cfgId();

        /**
         * return the Module type
         */
        MM_ModuleType moduleType();

        /**
         * Configure pins, load config etc.
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


    protected:
        /**
         * Type of the module
         * The module Type is submitted in the MOD_Type Message at startup and
         * to check if the config in EEPROM is for this module
         */
        MM_ModuleType _moduleType;

        /**
         * Target Port for Unicast
         */ 
        uint8_t _port = 0;

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
         * Targets for Multicast Messages
         */
        MM_Target _multicastTargets[MULTICAST_TARGETS];

        /**
         * Add a Target to the list
         * @param target and MM_CMD address to add
         * @return true if successful, false if target is already in list or list is full
         */
        bool addMulticastTarget(uint16_t addr, MM_CMD filter);

        /**
         * Remove a Target from the list
         * @param target address and MM_CMD to remove
         * @return true if successful, false if target isn't in list
         */
        bool removeMulticastTarget(uint16_t addr, MM_CMD filter);

        /**
         * Checks if a Target is in the list
         * @param target address and MM_CMD to check
         * @return true if Target is in list
         */
        bool checkMulticastTarget(uint16_t addr, MM_CMD filter);

        /**
         * Clears the whole list
         * @return true if successful
         */
        bool clearMulticastTargets();

        /**
         * store the targets in EEPROM
         */
        bool saveMulticastTargets();

        /**
         * load the targets from EEPROM
         */
        bool loadMulticastTargets();

        /**
         * Check if the Message is for this module(Unicast and Multicast)
         * if the message is a cfgMessage and this function can do the work returns false;
         * @param pkg the received Package
         * @return return true if the package is for the module
         */
        bool checkMsg(MM_Packet &pkg);
  
        /**
         * Write the config to EEPROM
         * @param config struct
         */
        template <typename T> bool writeConfig(const T& config){
            if(!_useEEPROM){
                #ifdef MM_DEBUG
                    Serial.println("EEPROM is deactivated!");
                #endif
                return false;
            }
            int eepromAddr = _controller->getEEPROMAddress(_cfgId);
            if(sizeof(config) > MAX_CONFIG_SIZE || eepromAddr+2+MAX_CONFIG_SIZE+sizeof(_multicastTargets) > EEPROM.length()){
                #ifdef MM_DEBUG
                    Serial.println("ERROR: Size of the config is to large!");
                #endif
                return false;
            }
    
            EEPROM.update(eepromAddr, _moduleType);
            EEPROM.put(eepromAddr + 1, config);
            return true;
        }
        /**
         * Read the config from EEPROM
         * @param config struct
         */
        template <typename T> bool readConfig(T& config){
            if(!_useEEPROM){
                #ifdef MM_DEBUG
                    Serial.println("EEPROM is deactivated!");
                #endif
                return false;
            }
            int eepromAddr = _controller->getEEPROMAddress(_cfgId);
            if(sizeof(config) > MAX_CONFIG_SIZE || eepromAddr+2+MAX_CONFIG_SIZE+sizeof(_multicastTargets) > EEPROM.length()){
                #ifdef MM_DEBUG
                    Serial.println("ERROR: Size of the config is to large!");
                #endif
                return false;
            } 
            if(EEPROM.read(eepromAddr) == _moduleType){
                EEPROM.get(eepromAddr + 1, config);
                return true;
            }else{
                #ifdef MM_DEBUG
                    Serial.println("ERROR: No config for this module found!");
                #endif
                return false;
            }
            
        }

        /**
         * Reset current configuration and reboot the node
         * @return bool false if failed
         */
        bool cfgReset();

        /**
         * Create an ErrorMsg an return to sender
         * @param pkg to return
         */
        void returnErrorMsg(MM_Packet &pkg); 
    
};

#endif