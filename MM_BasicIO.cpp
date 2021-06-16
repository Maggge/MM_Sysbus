#include "MM_BasicIO.h"

//------------ MM_Basic_Out --------------

MM_Digital_Out::MM_Digital_Out(uint8_t pin, uint8_t port, bool inverted){
    _pin = pin;
    _port = port;
    _config.inverted = inverted;
    _config.powerBack = ON_MODE::LastState;
    _moduleType = Digital_Out;
}

void MM_Digital_Out::begin(bool useEEPROM, uint8_t cfgId){
    MM_Module::begin(useEEPROM, cfgId); //Base class begin()
    
    pinMode(_pin, OUTPUT); //pin setup
    
    //Load the config from the EEPROM
    if(_useEEPROM){
        readConfig(_config);
    }

    //Recover the state
    switch (_config.powerBack)
    {
    case On:
        switchOutput(true);
        break;
    case LastState:
        switchOutput(_config.state);
        break;
    case Off:
        switchOutput(false);
        break;
    }
}

bool MM_Digital_Out::process(MM_Packet &pkg){
    if(checkMsg(pkg)){
        uint8_t len = 0;
        uint8_t data[8];
        switch (pkg.data[0]){
            case BOOL:
                switchOutput(bool(pkg.data[1]));
                break;
            case CFG_RESET:
                if(_controller == NULL) return;
                if(_useEEPROM){
                    EEPROM.update(_controller->getEEPROMAddress(_cfgId), 0);
                }
                data[0] = MM_CMD::ACK;
                data[1] = MM_CMD::CFG_RESET;
                _controller->Send(Broadcast, 0, 2, data);
                _controller->reboot();
                break;
            case CFG_REG_SET:
                if(pkg.data[1] == 1 && pkg.len == 3){
                    _config.inverted = bool(pkg.data[2]);
                    writeConfig(_config);
                    data[0] = MM_CMD::CFG_REG_COMMIT;
                    data[1] = 1;
                    data[2] = pkg.data[2];
                    len = 3;
                }
                else if(pkg.data[1] == 2 && pkg.len == 3){
                    _config.powerBack = ON_MODE(pkg.data[2]);
                    writeConfig(_config);
                    data[0] = MM_CMD::CFG_REG_COMMIT;
                    data[1] = 2;
                    data[2] = pkg.data[2];
                    len = 3;
                }
                else{
                    returnErrorMsg(pkg);
                }
                if(_controller != NULL){
                    _controller->Send(Broadcast, 0 , len, data);
                }
                break;
            case CFG_REG_GET:
                if(pkg.data[1] == 1){
                    data[0] = MM_CMD::CFG_REG_COMMIT;
                    data[1] = 1;
                    data[2] = _config.inverted;
                    len = 3;
                }
                else if(pkg.data[1] == 2){
                    data[0] = MM_CMD::CFG_REG_COMMIT;
                    data[1] = 2;
                    data[2] = _config.powerBack;
                    len = 3;
                }
                else{
                    returnErrorMsg(pkg);
                    return;
                }
                if(_controller != NULL){
                    _controller->Send(Broadcast, 0 , len, data);
                }
                break;
            default:
                break;
        }
    }
    return true;
}

bool MM_Digital_Out::loop(){
    return true;
}

bool MM_Digital_Out::broadcastState(){
    uint8_t data[2] = {BOOL, _config.state};
    if(_controller != NULL){
        return _controller->Send(Broadcast, 0, _port, 2, data);
    }
    return false;  
}

void MM_Digital_Out::switchOutput(bool power){
    if(power){
        digitalWrite(_pin, !_config.inverted);
    }
    else{
        digitalWrite(_pin, _config.inverted);    
    }
    _config.state = power;
    broadcastState();
    writeConfig(_config);
}