#include "MM_Module.h"
#include "MM_Sysbus.h"

void MM_Module::begin(bool useEEPROM, uint8_t cfgId){
    if(useEEPROM){
        int eepromAddr = _controller->getEEPROMAddress(cfgId);
        if(eepromAddr+1+MAX_CONFIG_SIZE+sizeof(_multicastTargets) <= EEPROM.length()){
            _useEEPROM = true;
            _cfgId = cfgId;
            if(EEPROM.read(eepromAddr) == _cfgId){
                loadMulticastTargets();
            }
        }
        else{
             #ifdef MM_DEBUG
                Serial.println("ERROR: End of EEPROM reached!");
            #endif
        }
    }
    broadcastModuleType();
}

bool MM_Module::cfgReset(){
    if(_useEEPROM){
        EEPROM.update(_controller->getEEPROMAddress(_cfgId), 0);
    }
    _controller->reboot();
    return false;
}

bool MM_Module::checkMsg(MM_Packet &pkg){

    if (pkg.meta.type == MM_MsgType::Streaming && pkg.meta.port == _port) {
        return true;
    }
    else if (pkg.meta.type == MM_MsgType::Multicast) {
        for (int i = 0; i < MULTICAST_TARGETS; i++) {
            if (_multicastTargets[i].address == pkg.meta.target && _multicastTargets[i].filter == (MM_CMD)pkg.data[0]) {
                return true;
            }
        }
        return false;
    }
    else if (pkg.meta.type == MM_MsgType::Unicast && pkg.meta.port == _port) {
        
        MM_Packet ack;
        ack.meta.type = MM_MsgType::Broadcast;
        uint16_t groupAddr = (uint16_t)pkg.data[1] << 8;
        groupAddr |= (uint16_t)pkg.data[2];

        switch (pkg.data[0])
        {
        case MM_CMD::REQ_TYPE:
            if(_controller != NULL){
                broadcastModuleType();
            }
            return false;
        case MM_CMD::GROUP_ADD:
            ack.len = 5;
            ack.data[1] = pkg.data[0];
            ack.data[2] = pkg.data[1]; 
            ack.data[3] = pkg.data[2];
            ack.data[4] = pkg.data[3];
            if(addMulticastTarget(groupAddr, (MM_CMD)pkg.data[3])){
                ack.data[0] = MM_CMD::ACK;
            }
            else{
                ack.data[0] = MM_CMD::ERROR;
            }
            break;
        case MM_CMD::GROUP_REM:
            ack.len = 5;
            ack.data[1] = pkg.data[0];
            ack.data[2] = pkg.data[1]; 
            ack.data[3] = pkg.data[2];
            ack.data[4] = pkg.data[3];
            if(removeMulticastTarget(groupAddr, (MM_CMD)pkg.data[3])){
                ack.data[0] = MM_CMD::ACK;
            }
            else{
                ack.data[0] = MM_CMD::ERROR;
            }
            break;
        case MM_CMD::GROUPS_CLEAR:
            ack.len = 2;
            ack.data[1] = pkg.data[0];
            if(clearMulticastTargets()){
                ack.data[0] = MM_CMD::ACK;
            }
            else{
                ack.data[0] = MM_CMD::ERROR;
            }
            break;
        case MM_CMD::GROUP_GET:{
            ack.len = 5;
            ack.data[0] = MM_CMD::GROUP_RETURN;
            ack.data[1] = pkg.data[1];
            ack.data[2] = highByte(_multicastTargets[pkg.data[1]].address);
            ack.data[3] = lowByte(_multicastTargets[pkg.data[1]].address);
            ack.data[4] = _multicastTargets[pkg.data[1]].filter;
            break;
        }
        default:
            return true;
        }
        _controller->Send(ack);
        return true;
    }
    else if(pkg.meta.type == MM_MsgType::Broadcast){
        if(pkg.data[0] == MM_CMD::REQ_TYPE){
            broadcastModuleType();
        }
        return false;
    }
    else {
        return false;
    }
}

uint8_t MM_Module::port(){
    return _port;
}

void MM_Module::broadcastModuleType(){
    if(_controller == NULL) return;
    uint8_t typeMsg[] = {MOD_TYPE, _moduleType, _useEEPROM};
    _controller->Send(Broadcast, 0, _port, 3, typeMsg);
}

template <class T> bool MM_Module::writeConfig(const T& config){
    if(!_useEEPROM){
        #ifdef MM_DEBUG
            Serial.println("EEPROM is deactivated!");
        #endif
        return false;
    }

    if(sizeof(config) > MAX_CONFIG_SIZE){
        #ifdef MM_DEBUG
            Serial.println("ERROR: Size of the config is to large!");
        #endif
        return false;
    }

    int eepromAddr = _controller->getEEPROMAddress(_cfgId);
    return (EEPROM_write(eepromAddr, _moduleType) && EEPROM_write(eepromAddr + 1, config));
}

template <class T> bool MM_Module::readConfig(T& config){
    if(!_useEEPROM){
        #ifdef MM_DEBUG
            Serial.println("EEPROM is deactivated!");
        #endif
        return false;
    }
    if(sizeof(config) > MAX_CONFIG_SIZE){
        #ifdef MM_DEBUG
            Serial.println("ERROR: Size of the config is to large!");
        #endif
        return false;
    }
    int eepromAddr = _controller->getEEPROMAddress(_cfgId);
    if(EEPROM.read(eepromAddr) == _moduleType){
        return EEPROM_read(eepromAddr + 1, config);
    }else{
        #ifdef MM_DEBUG
            Serial.println("ERROR: No config for this module found!");
        #endif
        return false;
    }
    
}

//-----------MulticastTargets---------------------

bool MM_Module::addMulticastTarget(uint16_t addr, MM_CMD filter){
    for (int i = 0; i < MULTICAST_TARGETS; i++) {
        if (_multicastTargets[i].address == addr && _multicastTargets[i].filter == filter){
            #ifdef MM_DEBUG
                Serial.print("Multicast target already exist: ");
                Serial.print(addr);
                Serial.print(",  ");
                Serial.println(filter);
            #endif
            return true;
        }
    }
    MM_Target t;
    t.address = addr;
    t.filter = filter;
    for (int i = 0; i < MULTICAST_TARGETS; i++) {
        if (_multicastTargets[i].address == NULL && _multicastTargets[i].filter == NULL) {
            _multicastTargets[i] = t;
            #ifdef MM_DEBUG
                Serial.print("Add Multicast target: ");
                Serial.print(addr);
                Serial.print(",  ");
                Serial.println(filter);
            #endif
            if(_useEEPROM){
                return saveMulticastTargets();
            }
            else{
                return true;
            }
        }
    }
    #ifdef MM_DEBUG
        Serial.print("Multicast targets full ");
    #endif
    return false;
}

bool MM_Module::removeMulticastTarget(uint16_t addr, MM_CMD filter){
    for (int i = 0; i < MULTICAST_TARGETS; i++) {
        if (_multicastTargets[i].address == addr && _multicastTargets[i].filter == filter) {
            #ifdef MM_DEBUG
                Serial.print("Remove Multicast target: ");
                Serial.print(addr);
                Serial.print(",  ");
                Serial.println(filter);
            #endif
            _multicastTargets[i].address = NULL;
            _multicastTargets[i].filter = NULL;
            if(_useEEPROM){
                return saveMulticastTargets();
            }
            else{
                return true;
            }
        }
    }
    #ifdef MM_DEBUG
        Serial.print("Multicast target not found: ");
        Serial.print(addr);
        Serial.print(",  ");
        Serial.println(filter);
    #endif
    return true;
}

bool MM_Module::checkMulticastTarget(uint16_t addr, MM_CMD filter){
    for (int i = 0; i < MULTICAST_TARGETS; i++) {
        if (_multicastTargets[i].address == addr && _multicastTargets[i].filter == filter) {
            return true;
        }
    }
    return false;
}

bool MM_Module::clearMulticastTargets(){
    #ifdef MM_DEBUG
        Serial.println("Clear Multicast targets...");
    #endif
    for (int i = 0; i < MULTICAST_TARGETS; i++) {
        _multicastTargets[i].address = NULL;
        _multicastTargets[i].filter = NULL;
    }
    if(_useEEPROM){
        return saveMulticastTargets();
    }
    else{
        return true;
    }
}

bool MM_Module::saveMulticastTargets(){
    if(!_useEEPROM){
        #ifdef MM_DEBUG
            Serial.println("EEPROM is deactivated!");
        #endif
        return false;
    }
    int eepromAddr = _controller->getEEPROMAddress(_cfgId) + 1 + MAX_CONFIG_SIZE;
    EEPROM_write(eepromAddr, _multicastTargets);

}

bool MM_Module::loadMulticastTargets(){
    if(!_useEEPROM){
        #ifdef MM_DEBUG
            Serial.println("EEPROM is deactivated!");
        #endif
        return false;
    }
    int eepromAddr = _controller->getEEPROMAddress(_cfgId) + 1 + MAX_CONFIG_SIZE;
    EEPROM_read(eepromAddr, _multicastTargets);
}

//--------------EEPROM--------------------

template <class T> bool MM_Module::EEPROM_write(int eepromAddr, T& data){
    const byte* p = (const byte*)(const void*)&data;
    unsigned int i;
    for (i = 0; i < sizeof(data); i++)
          EEPROM.update(eepromAddr++, *p++);
    return true;
}

template <class T> bool MM_Module::EEPROM_read(int eepromAddr, T& data){
    byte* p = (byte*)(void*)&data;
    unsigned int i;
    for (i = 0; i < sizeof(data); i++)
          *p++ = EEPROM.read(eepromAddr++);
    return true;
}