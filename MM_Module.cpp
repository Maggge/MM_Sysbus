#include "MM_Module.h"
#include "MM_Sysbus.h"

void MM_Module::begin(bool useEEPROM, uint8_t cfgId){
    _cfgId = cfgId;
    if(useEEPROM){
        _useEEPROM = true;        
        loadMulticastTargets();
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
        #ifdef MM_DEBUG
            Serial.println("Check Multicast");
            Serial.println("Multicast-Targets:");
            for(int i = 0; i < MULTICAST_TARGETS; i++){
                Serial.print(i);
                Serial.print(": ");
                Serial.print(_multicastTargets[i].address);
                Serial.print("  ");
                Serial.println(_multicastTargets[i].filter);
            }
        #endif
        for (int i = 0; i < MULTICAST_TARGETS; i++) {
            if (_multicastTargets[i].address == pkg.meta.target && (_multicastTargets[i].filter == (MM_CMD)pkg.data[0] || _multicastTargets[i].filter == MM_CMD::ALL_CMDS)){
                return true;
            }
        }
        return false;
    }
    else if (pkg.meta.type == MM_MsgType::Unicast && pkg.meta.port == _port) {
        if(pkg.data[0] == REQ_TYPE){
            if(_controller != NULL){
                broadcastModuleType();
            }
            return false;
        }
        else if(pkg.data[0] == GROUP_ADD || pkg.data[0] == GROUP_REM){
            if(pkg.len < 3 || pkg.len > 4){
                returnErrorMsg(pkg);
                return false;
            }

            uint16_t groupAddr = (uint16_t)pkg.data[1] << 8 | (uint16_t)pkg.data[2];
            MM_CMD filter = ALL_CMDS;          
            if(pkg.len == 4){
                filter = (MM_CMD)pkg.data[3];
            }

            bool successfull;
            if(pkg.data[0] == GROUP_ADD){
                successfull = addMulticastTarget(groupAddr, filter);
            }
            else{
                successfull = removeMulticastTarget(groupAddr, filter);
            }
            if(successfull){
                if(_controller != NULL){
                    uint8_t data[8];
                    data[0] = MM_CMD::ACK;
                    for (int i = 0; i < pkg.len; i++){
                        data[i+1] = pkg.data[i];
                    }
                    uint8_t len;
                    if(pkg.len == 3){
                        data[4] = filter;
                        len = 5;
                    }else{
                        len = 4;
                    }
                
                    _controller->Send(MM_MsgType::Broadcast, pkg.meta.source, len, data);
                }
                return false;
            }
            else{
                returnErrorMsg(pkg);
            }
            return false;
        }
        else if(pkg.data[0] == GROUP_GET){
            if(_controller != NULL){
                MM_Target req = _multicastTargets[pkg.data[1]];
                uint8_t data[8];
                data[0] = GROUP_RETURN;
                data[1] = pkg.data[1];
                data[2] = highByte(req.address);
                data[3] = lowByte(req.address);
                data[4] = req.filter;
                _controller->Send(MM_MsgType::Broadcast, pkg.meta.source, 5, data);
            }
            return false;
        }
        else if(pkg.data[0] == GROUPS_CLEAR){
            clearMulticastTargets();
            if(_controller != NULL){
                uint8_t data[] = {MM_CMD::ACK, MM_CMD::GROUPS_CLEAR};
                _controller->Send(MM_MsgType::Broadcast, pkg.meta.source, 2, data);
            }
        }
        else{
            return true;
        }
    }
    else{
        return false;
    }
}

uint8_t MM_Module::port(){
    return _port;
}

uint8_t MM_Module::cfgId(){
    return _cfgId;
}

MM_ModuleType MM_Module::moduleType(){
    return _moduleType;
}

void MM_Module::broadcastModuleType(){
    if(_controller == NULL) return;
    uint8_t typeMsg[] = {MOD_TYPE, _moduleType, _useEEPROM};
    _controller->Send(Broadcast, 0, _port, 3, typeMsg);
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
            #ifdef MM_DEBUG
                Serial.println("Multicast-Targets:");
                for(int i = 0; i < MULTICAST_TARGETS; i++){
                    Serial.print(i);
                    Serial.print(": ");
                    Serial.print(_multicastTargets[i].address);
                    Serial.print("  ");
                    Serial.println(_multicastTargets[i].filter);
                }
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
            #ifdef MM_DEBUG
                Serial.println("Multicast-Targets:");
                for(int i = 0; i < MULTICAST_TARGETS; i++){
                    Serial.print(i);
                    Serial.print(": ");
                    Serial.print(_multicastTargets[i].address);
                    Serial.print("  ");
                    Serial.println(_multicastTargets[i].filter);
                }
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
    if(eepromAddr+1+sizeof(_multicastTargets) > EEPROM.length()){
        return false;
    }
    EEPROM.update(eepromAddr, _moduleType);
    eepromAddr++;
    for(int i = 0; i < MULTICAST_TARGETS; i++){
        EEPROM.put(eepromAddr, _multicastTargets[i]);
        eepromAddr += sizeof(MM_Target);
    }
    return true;
}

bool MM_Module::loadMulticastTargets(){
    if(!_useEEPROM){
        #ifdef MM_DEBUG
            Serial.println("EEPROM is deactivated!");
        #endif
        return false;
    }
    int eepromAddr = _controller->getEEPROMAddress(_cfgId) + 1 + MAX_CONFIG_SIZE;
    if(eepromAddr+1+sizeof(_multicastTargets) > EEPROM.length()){
        return false;
    }
    if(EEPROM.read(eepromAddr) == _moduleType){
        eepromAddr++;
        for(int i = 0; i < MULTICAST_TARGETS; i++){
            EEPROM.get(eepromAddr, _multicastTargets[i]);
            eepromAddr += sizeof(MM_Target);
        }
        return true;
    }
    return false;
}

void MM_Module::returnErrorMsg(MM_Packet &pkg){
    if(_controller != NULL){
        uint8_t data[8];
        data[0] = MM_CMD::ERROR;
        for(int i = 0; i < pkg.len; i++){
            data[i+1] = pkg.data[i];
        }
        
        _controller->Send(MM_MsgType::Unicast, pkg.meta.source, pkg.len+1, data);  
    }
}