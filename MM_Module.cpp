#include "MM_Module.h"
#include "MM_Sysbus.h"

void MM_Module::begin(bool useEEPROM, uint8_t cfgId){
    if(useEEPROM){
        _useEEPROM = true;
        _cfgId = cfgId;
        _register.eepromAddress = _controller->getEEPROMAddress(&_cfgId);
        _register.addRegister(&_moduleType);
        _multicastTargets.useRegister(&_register);
        _register.loadRegister();
    }
    broadcastModuleType();
}

bool MM_Module::cfgReset(){
    _multicastTargets.clearTargets();
    _register.clearRegister();
    return true;
}

bool MM_Module::checkMsg(MM_Packet &pkg){
    bool forMe = false;

    if(pkg.meta.type == Broadcast && pkg.data[0] == REQ_PRES){
        broadcastModuleType();
        return false;
    }

    if (pkg.meta.type == Multicast) {
        for (int i = 0; i < MULTICAST_TARGETS; i++) {
            if (_multicastTargets._targets[i] == pkg.meta.target) {
                forMe = true;
                break;
            }
        }
    }
    else if ((pkg.meta.type == Unicast || pkg.meta.type == Streaming) && pkg.meta.port == _port) {
        forMe = true;
    }

    if (forMe) {
        if (pkg.meta.type != Unicast) {
            return true;
        }
        uint8_t data[8];
        uint16_t addr = (uint16_t)pkg.data[1] << 8;
        addr |= (uint16_t)pkg.data[2];

        switch (pkg.data[0])
        {
        case REQ_PRES:
            broadcastModuleType();
            return false;
        case GROUP_ADD:
            if(_multicastTargets.addTarget(addr)){
                for(int i = 0; i < pkg.len; i++){
                    data[i] = pkg.data[i];
                }
            }
            else{
                data[0] = ERROR;
                for(int i = 0; i < pkg.len; i++){
                    data[i+1] = pkg.data[i];
                }
                pkg.len++;
            }
            if(_controller != NULL){
                _controller->Send(Broadcast, pkg.meta.source, pkg.len, data);
            }
            return false;
        case GROUP_REM:
            if(_multicastTargets.removeTarget(addr)){
                for(int i = 0; i < pkg.len; i++){
                    data[i] = pkg.data[i];
                }
            }
            else{
                data[0] = ERROR;
                for(int i = 0; i < pkg.len; i++){
                    data[i+1] = pkg.data[i];
                }
                pkg.len++;
            }
            if(_controller != NULL){
                _controller->Send(Broadcast, pkg.meta.source, pkg.len, data);
            }
            return false;
        case GROUP_CLEAR:
            if(_multicastTargets.clearTargets()){
                for(int i = 0; i < pkg.len; i++){
                    data[i] = pkg.data[i];
                }
            }
            else{
                data[0] = ERROR;
                for(int i = 0; i < pkg.len; i++){
                    data[i+1] = pkg.data[i];
                }
                pkg.len++;
            }
            if(_controller != NULL){
                _controller->Send(Broadcast, pkg.meta.source, pkg.len, data);
            }
            return false;
        case REG_SET:
            uint8_t regsToSet = pkg.len - 1;
            uint8_t r[regsToSet];
            for(int i = 0; i < regsToSet; i++){
                r[i] = pkg.data[i+2];
            }
            if(_register.setRegister(pkg.data[1], regsToSet, r)){
                if(_controller != NULL){
                    data[0] = REG_COMMIT;
                    data[1] = pkg.data[1];
                    for(int i = 0; i < regsToSet; i++){
                        data[i+2] = _register.getRegister(pkg.data[1] + i);
                    }
                    _controller->Send(Unicast, pkg.meta.source, pkg.len, data);
                }
            }else{
                if(_controller != NULL){
                    data[0] = ERROR;
                   _controller->Send(Unicast, pkg.meta.source, 1, data);
                } 
            }
            return false;
        case REG_GET:
            if(_controller != NULL){
                data[0] = REG_COMMIT;
                data[1] = pkg.data[1];
                uint8_t regsToRead = pkg.data[2];

                if(pkg.data[1] >= REGISTER_PER_MODUL){
                    data[0] = ERROR;
                    _controller->Send(Unicast, pkg.meta.source, 1, data);
                } 
                else if(pkg.data[1] + pkg.data[2] >= REGISTER_PER_MODUL){
                    regsToRead = REGISTER_PER_MODUL - pkg.data[1];
                }
                int j = 2;
                for(int i = 0; i < regsToRead; i++){
                    data[j] = _register.getRegister(pkg.data[1] + i);
                    j++;
                    if(j > 7 || i + 1 == regsToRead){
                        _controller->Send(pkg.meta.type, pkg.meta.source, j, data);
                        data[1] = data[1] + 6;
                        j = 2;
                    }
                }  
            }
            return false;
        default:
            return true;
        }



    }
    else {
        return false;
    }
}

void MM_Module::broadcastModuleType(){
    if(_controller == NULL) return;
    uint8_t typeMsg[] = {MOD_TYPE, _moduleType, _useEEPROM};
    _controller->Send(Broadcast, 0, _port, 3, typeMsg);
}


//--------------Register--------------------------

template<typename T> 
bool Register::addRegister(const T *reg){
    if(eepromAddress + REGISTER_PER_MODUL > EEPROM.length()) return false;
    uint8_t *r = (uint8_t*)reg;
    if(_regsCount + sizeof(T) <= REGISTER_PER_MODUL){
        for(int i = 0; i < sizeof(T); i++){
            _regs[_regsCount] = &r[i];
            #ifdef MM_DEBUG
                Serial.print("Add Reg ");
                Serial.print(i);
                Serial.print(": ");
                Serial.println(*_regs[_regsCount]);
            #endif
            _regsCount++;
        }
        return true;
    }
    return false;
}

bool Register::setRegister(uint8_t index, uint8_t len, uint8_t* data){
    if(index + len >= REGISTER_PER_MODUL) return false;
    for(int i = 0; i < len; i++){
        *_regs[index+i] = data[i];
    }
    return saveRegister();
}

bool Register::saveRegister(){
    if(eepromAddress + REGISTER_PER_MODUL > EEPROM.length()) return false;
    for(int i = 0; i < _regsCount; i++){
        EEPROM.update(eepromAddress + i, *_regs[i]);
        #ifdef MM_DEBUG
            Serial.print("Reg ");
            Serial.print(i);
            Serial.print(": ");
            Serial.println(*_regs[i]);
        #endif
    }
    return true;
}

bool Register::loadRegister(){
    if(eepromAddress + REGISTER_PER_MODUL > EEPROM.length()){
        #ifdef MM_DEBUG
            Serial.print("EEPROM Address is too high!");
        #endif
        return false;
    }
    if(EEPROM.read(eepromAddress) != *_regs[0]){
        #ifdef MM_DEBUG
            Serial.print("Config in EEPROM at this Address is not for this Module!");
        #endif
        return false;
    }
    for(int i = 1; i < _regsCount; i++){
        *_regs[i] = EEPROM.read(eepromAddress + i);
        #ifdef MM_DEBUG
            Serial.print("Load Reg ");
            Serial.print(i);
            Serial.print(": ");
            Serial.println(*_regs[i]);
        #endif
    }
    return true;
}

bool Register::clearRegister(){
    if(eepromAddress + REGISTER_PER_MODUL > EEPROM.length()) return false;
    for(int i = 0; i < REGISTER_PER_MODUL; i++){
        EEPROM.update(eepromAddress + i, 0);
    }
    return true;
}

//-----------MulticastTargets---------------------

void MulticastTargets::useRegister(Register *reg){
    if(reg == NULL) return;
    _reg = reg;
    _reg->addRegister(_targets);
}

bool MulticastTargets::addTarget(uint16_t target){
    for (int i = 0; i < MULTICAST_TARGETS; i++) {
        if (_targets[i] == target){
            #ifdef MM_DEBUG
                Serial.print("Multicast target already exist: ");
                Serial.print(target);
            #endif
            return false;
        }
    }
    for (int i = 0; i < MULTICAST_TARGETS; i++) {
        if (_targets[i] == 0) {
            _targets[i] = target;
            #ifdef MM_DEBUG
                Serial.print("Add Multicast target: ");
                Serial.print(target);
            #endif
            if(_reg == NULL) return true;
            return _reg->saveRegister();
        }
    }
    #ifdef MM_DEBUG
        Serial.print("Multicast targets full ");
    #endif
    return false;
}

bool MulticastTargets::removeTarget(uint16_t target){
    for (int i = 0; i < MULTICAST_TARGETS; i++) {
        if (_targets[i] == target) {
            #ifdef MM_DEBUG
                Serial.print("Remove Multicast target: ");
                Serial.print(target);
            #endif
            _targets[i] = 0;
            if(_reg == NULL) return true;
            return _reg->saveRegister();
        }
    }
    #ifdef MM_DEBUG
        Serial.print("Multicast target not found: ");
        Serial.print(target);
    #endif
    return false;
}

bool MulticastTargets::checkTarget(uint16_t target){
    for (int i = 0; i < MULTICAST_TARGETS; i++) {
        if (_targets[i] == target) {
            return true;
        }
    }
    return false;
}

bool MulticastTargets::clearTargets(){
    #ifdef MM_DEBUG
        Serial.println("Clear Multicast targets...");
    #endif
    for (int i = 0; i < MULTICAST_TARGETS; i++) {
        _targets[i] = 0;
    }
    if(_reg == NULL) return true;
    return _reg->saveRegister();
}