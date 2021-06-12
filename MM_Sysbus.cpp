#include "MM_Sysbus.h"

MM_Sysbus::MM_Sysbus(uint16_t nodeID) {
    _useEEPROM = false;
    setNodeId(nodeID);
    _firstboot = true;
    #ifdef MM_DEBUG
        Serial.print("Node initailized with ID: ");
        Serial.println(_nodeID);
    #endif
}

MM_Sysbus::MM_Sysbus(uint16_t nodeID, uint16_t EEPROMstart){
    _useEEPROM = true;
    _EEPROMaddr = EEPROMstart;
    setNodeId(nodeID);

    uint8_t cfg;
    EEPROM.get(_EEPROMaddr, cfg);
    if (cfg == 99) {
        EEPROM.get(_EEPROMaddr + 1, _nodeID);
        _initialized = true;
        _firstboot = false;
    }
    else{
        setNodeId(nodeID);
        _firstboot = true;
    }
    #ifdef MM_DEBUG
        Serial.print("Node initailized with ID: ");
        Serial.println(_nodeID);
    #endif
}

MM_Sysbus::MM_Sysbus(uint8_t cfgButton, uint8_t statusLED, uint16_t EEPROMstart){
    _useEEPROM = true;
    _EEPROMaddr = EEPROMstart;

    _cfgBtn = &ConfigButton(cfgButton, 5000);
    _statusLED = statusLED;
    pinMode(_statusLED, OUTPUT);

    uint8_t cfg;
    EEPROM.get(_EEPROMaddr, cfg);
    if (cfg == 99) {
        EEPROM.get(_EEPROMaddr + 1, _nodeID);
        _initialized = true;
        _firstboot = false;
        #ifdef MM_DEBUG
            Serial.print("Node initailized with ID: ");
            Serial.println(_nodeID);
        #endif
    }
    else
    {
        _initialized = false;
        _firstboot = true;
    }
}

bool MM_Sysbus::firstboot(void (*function)()){
    if(_firstboot && function != NULL){
        #ifdef MM_DEBUG
            Serial.print("Call firstboot function.");
        #endif
        function();
        return true;
    }
    return false;
    
}

bool MM_Sysbus::setNodeId(uint16_t nodeID){
    #ifdef MM_DEBUG
        Serial.print("Request ID: ");
        Serial.println(nodeID);
    #endif
    if(nodeID < 0 || nodeID > 2047){
        return false;
    }
    _nodeID = nodeID;
    _initialized = true;
    #ifdef MM_DEBUG
        Serial.print("Set new Node ID: ");
        Serial.println(_nodeID);
    #endif
    if(_useEEPROM){
        EEPROM.update(_EEPROMaddr, (uint8_t)99);
        EEPROM.put(_EEPROMaddr + 1, _nodeID);
    }
   
    uint8_t data[1] = { MM_CMD::NODE_BOOT };
    Send(MM_MsgType::Broadcast, 0x00, 1, data);

    for (int i = 0; i < MAX_MODULES; i++) {
        if (_modules[i] != NULL) {
            _modules[i]->broadcastModuleType();
        }
    }
    
    return true;
}

uint16_t MM_Sysbus::nodeID(){
    return _nodeID;
}

bool MM_Sysbus::attachBus(MM_Interface* bus){
    for(int i = 0; i < MAX_INTERFACES; i++){
        if(_interfaces[i] == bus){
            #ifdef MM_DEBUG
                Serial.println("Bus already attached");
            #endif
            return false;
        }
    }
    for(int busId = 0; busId < MAX_INTERFACES; busId++){
        if(_interfaces[busId] == NULL){
            _interfaces[busId] = bus;

            if (bus->begin()) {
                if(_initialized){
                    //Boot message
                    uint8_t data[1] = { MM_CMD::NODE_BOOT };
                    bus->Send(MM_MsgType::Broadcast, 0x00, _nodeID, 0, sizeof(data), data);
                }
                #ifdef MM_DEBUG
                    Serial.println("Bus attached");
                #endif
                return true;
            }else{
                _interfaces[busId] = NULL;
                #ifdef MM_DEBUG
                    Serial.println("Bus error!");
                #endif
                return false;
            }
        }
    }
    #ifdef MM_DEBUG
        Serial.println("Max Buses attached");
    #endif
    return false;
}

bool MM_Sysbus::detachBus(MM_Interface* bus){
    for(int i = 0; i < MAX_INTERFACES; i++){
        if(_interfaces[i] == bus){
            _interfaces[i] = NULL;
            #ifdef MM_DEBUG
                Serial.println("Bus detached");
            #endif
            return true;
        }
    }
    #ifdef MM_DEBUG
        Serial.println("Bus isn't attached");
    #endif
    return false;
}


bool MM_Sysbus::Send(MM_Packet pkg){
    bool allSuccesfull = true;

    for (signed char busId = 0; busId < MAX_INTERFACES; busId++) {
        if (_interfaces[busId] != NULL && busId != pkg.meta.busId) {
            if(!_interfaces[busId]->Send(pkg.meta.type, pkg.meta.target, pkg.meta.source, pkg.meta.port, pkg.len, pkg.data)){
                allSuccesfull = false;
                #ifdef MM_DEBUG
                Serial.println("---Send-Error---");
                Serial.print("Interface ID: ");
                Serial.println(busId);
                #endif
            }
        }
    }

    if(pkg.meta.busId < 0){
        //The message is sent from local source, check for actions
        if(_initialized && _nodeID != 0){
            Process(pkg);
        }
    }

    #ifdef MM_DEBUG
        Serial.println("---Message Sent---");
        Serial.print("Type: ");
        Serial.println(pkg.meta.type);
        Serial.print("Target: ");
        Serial.println(pkg.meta.target);
        Serial.print("Port: ");
        Serial.println((uint8_t)pkg.meta.port);
        Serial.print("Source: ");
        Serial.println(pkg.meta.source);
        Serial.print("Data Lenght: ");
        Serial.println((uint8_t)pkg.len);
        Serial.print("Data:");
        for (int i = 0; i < pkg.len; i++) {
            Serial.print("  ");
            Serial.print(pkg.data[i]);
        }
        Serial.println();
    #endif

    return allSuccesfull;
}

bool MM_Sysbus::Send(MM_Meta meta, uint8_t len, uint8_t *data){
    return Send(meta.type, meta.target, meta.source, meta.port, len, data, meta.busId);
}

bool MM_Sysbus::Send(MM_MsgType msgType, uint16_t target, uint8_t len, uint8_t *data){
    return Send(msgType, target, _nodeID, 0, len, data, -1);
}

bool MM_Sysbus::Send(MM_MsgType msgType, uint16_t target, uint8_t port, uint8_t len, uint8_t *data){
    return Send(msgType, target, _nodeID, port, len, data, -1);
}

bool MM_Sysbus::Send(MM_MsgType msgType, uint16_t target, uint16_t source, uint8_t port, uint8_t len, uint8_t *data, signed char skipInterface){
    MM_Packet pkg;

    pkg.meta.type = msgType;
    pkg.meta.target = target;
    pkg.meta.source = source;
    pkg.meta.port = port;
    pkg.len = len;
    for (int i = 0; i < len; i++) pkg.data[i] = data[i];
    pkg.meta.busId = skipInterface;

    return Send(pkg);
}

bool MM_Sysbus::Receive(MM_Packet &pkg){
    return Receive(pkg, true);
}

bool MM_Sysbus::Receive(MM_Packet &pkg, bool routing){
    bool check = false;

    for (signed char busId = 0; busId < MAX_INTERFACES; busId++) {
        if (_interfaces[busId] != NULL) {
            check = _interfaces[busId]->Receive(pkg);
            if (check) {
                pkg.meta.busId = busId;
                #ifdef MM_DEBUG
                    Serial.println("---Message Received---");
                    Serial.print("Type: ");
                    Serial.println(pkg.meta.type);
                    Serial.print("Target: ");
                    Serial.println(pkg.meta.target);
                    Serial.print("Port: ");
                    Serial.println((uint8_t)pkg.meta.port);
                    Serial.print("Source: ");
                    Serial.println(pkg.meta.source);
                    Serial.print("Data Lenght: ");
                    Serial.println((uint8_t)pkg.len);
                    for (int i = 0; i < pkg.len; i++) {
                        Serial.println(pkg.data[i]);
                    }
                #endif
                if (routing) {
                    //Resend to every attached interface except the one we received it on (pkg.meta.busId)
                    Send(pkg);
                }

                if(_initialized && _nodeID != 0){
                    Process(pkg);
                }
                return true;
            }
        }
    }
    return false;
}

void MM_Sysbus::Process(MM_Packet& pkg) {
    uint8_t i;
    uint8_t data[8];

    //attached hooks
    for (i = 0; i < MAX_HOOKS; i++) {
        if (_hooks[i].execute != NULL) {
            if (
                (_hooks[i].type == pkg.meta.type) &&
                (_hooks[i].target == 0 || _hooks[i].target == pkg.meta.target) &&
                (_hooks[i].port == -1  || _hooks[i].port == pkg.meta.port) &&
                (_hooks[i].cmd == MM_CMD::ALL_CMDS  || (pkg.len > 0 && _hooks[i].cmd == pkg.data[0]))){
                _hooks[i].execute(pkg);
            }
        }
    }
 
    //Internal logic
    if (pkg.len >= 1) {
        MM_CMD cmd = (MM_CMD)pkg.data[0];
        switch (cmd) {
            case NODE_PING:
                if (pkg.meta.type != MM_MsgType::Unicast || pkg.meta.target != _nodeID) break;  
                data[0] = MM_CMD::NODE_PONG;
                Send(MM_MsgType::Unicast, pkg.meta.source, _nodeID, pkg.meta.port, 1, data, -1);
                break;
            case NODE_PONG:
                break;
            case REQ_TYPE:
                if(pkg.meta.type == MM_MsgType::Broadcast){
                    for (int i = 0; i < MAX_MODULES; i++) {
                        if (_modules[i] != NULL) {
                            _modules[i]->broadcastModuleType();
                        }
                    }
                }
                else if(pkg.meta.type == MM_MsgType::Unicast && pkg.meta.target == _nodeID){
                    for (int i = 0; i < MAX_MODULES; i++) {
                        if (_modules[i] != NULL && _modules[i]->port() == pkg.meta.port) {
                            _modules[i]->broadcastModuleType();
                        }
                    }
                }
                break;
            case RESET_NODE:
                if (pkg.meta.type != MM_MsgType::Unicast || pkg.meta.target != _nodeID){
                    break;
                }    
                reset();
                break;
            case NODE_ID:
                if(!_initialized || pkg.meta.target != _nodeID || pkg.len != 3) break;
                setNodeId((pkg.data[1] << 8) | (pkg.data[2]));
                break;

            default:
                break;
        }
    }

    //attached modules
    if ( pkg.meta.type == MM_MsgType::Multicast ) {
        for (int i = 0; i < MAX_MODULES; i++) {
            if (_modules[i] != NULL) {
                _modules[i]->process(pkg);
            }
        }
        return;
    }
    else if((pkg.meta.type == MM_MsgType::Unicast || pkg.meta.type == MM_MsgType::Streaming) && pkg.meta.target == _nodeID){
        for (int i = 0; i < MAX_MODULES; i++) {
            if (_modules[i] != NULL && _modules[i]->port()) {
                _modules[i]->process(pkg);
                return;
            }
        }
    }
}

bool MM_Sysbus::attachModule(MM_Module *module, uint8_t cfgId){
    for(int i = 0; i < MAX_MODULES; i++){
        if(_modules[i]->port() == module->port()){
            #ifdef MM_DEBUG
                Serial.println("Port already in use");
            #endif
            return false;
        }
    }

    if(_modules[cfgId] == NULL){
        _modules[cfgId] = module;
        module->_controller = this;
        module->begin(_useEEPROM, cfgId);
        #ifdef MM_DEBUG
            Serial.println("Module attached");
        #endif
        return true;
    }else
    {
        #ifdef MM_DEBUG
            Serial.println("Module slot already occupied!");
        #endif
        return false;
    }
}

uint8_t MM_Sysbus::attachModule(MM_Module *module){
    for(int i = 0; i < MAX_MODULES; i++){
        if(_modules[i] == NULL){
            if(attachModule(module, i)){
                return i;
            }
            return 255;
        }
    }
    #ifdef MM_DEBUG
        Serial.println("Max Modules added!");
    #endif
    return 255;
}

bool MM_Sysbus::detachModule(MM_Module *module){
    for(int i = 0; i < MAX_MODULES; i++){
        if(_modules[i] == module){
            _modules[i] = NULL;
            #ifdef MM_DEBUG
                Serial.println("Module detached");
            #endif
            return true;
        }
    }
    #ifdef MM_DEBUG
        Serial.println("Module not found");
    #endif
    return false;
}

bool MM_Sysbus::attachHook(MM_MsgType msgType, uint16_t target, uint8_t port, MM_CMD cmd, void (*function)(MM_Packet&)) {
    for (byte i = 0; i < MAX_HOOKS; i++) {
        if (_hooks[i].execute == 0) {
            _hooks[i].type = msgType;
            _hooks[i].target = target;
            _hooks[i].port = port;
            _hooks[i].cmd = cmd;
            _hooks[i].execute = function;
            return true;
        }
    }
    return false;
}

MM_Packet MM_Sysbus::loop(void) {
    MM_Packet pkg;

    //Packet handling
    Receive(pkg);

    if(_cfgBtn != NULL){
        if (_cfgBtn->process() == ButtonState::Long_push) {
            initialization();
        }
    }
    
    if (!_initialized) {
        digitalWrite(_statusLED, HIGH);
        return pkg;
    }    

    //Modules loop
    for (int i = 0; i < MAX_MODULES; i++) {
        if (_modules[i] != NULL) {
            _modules[i]->loop();
        }
    }

    return pkg;
}

void MM_Sysbus::initialization() {
    MM_Packet pkg;
    uint32_t exitTime = millis() + 60000;
    while (exitTime >= millis()) {

        if(_cfgBtn != NULL){
            if (_cfgBtn->process() == ButtonState::Long_push) {
                reset();
            }
        }

        if (millis() % 1000 > 500) {
            digitalWrite(_statusLED, HIGH);
        }
        else {
            digitalWrite(_statusLED, LOW);
        }

        if (Receive(pkg) && pkg.data[0] == NODE_ID && pkg.len == 1) {
            setNodeId(pkg.meta.target); 
            Send(MM_MsgType::Broadcast, pkg.meta.source, pkg.len, pkg.data);
            _initialized = true;
            for (int i = 0; i < 10; i++){
                delay(100);
                digitalWrite(_statusLED, HIGH);
                delay(100);
                digitalWrite(_statusLED, LOW);
            }
            return;
        }
    }

}

uint16_t MM_Sysbus::getEEPROMAddress(uint8_t cfgID) {
    return (cfgID * (MAX_CONFIG_SIZE + 1 + (sizeof(MM_Target) * MULTICAST_TARGETS)) + _EEPROMaddr + 3);
}

void MM_Sysbus::reset(){
    #ifdef MM_DEBUG
        Serial.println("Reset");
    #endif    
    if(_statusLED != 0){
        for (int i = 0; i < 10; i++){
            delay(50);
            digitalWrite(_statusLED, HIGH);
            delay(50);
            digitalWrite(_statusLED, LOW);
        }
    }
    if(_useEEPROM){
        for (int i = 0 ; i < EEPROM.length() ; i++) {
            EEPROM.write(i, 0);
        }
    }
    reboot();
}

void MM_Sysbus::reboot(){
    #ifdef MM_DEBUG
        Serial.println("Reboot");
    #endif
    delay(100);
    wdt_enable(WDTO_15MS);
    while (true);  
}

ConfigButton::ConfigButton(uint8_t buttonPin, uint16_t t_long_press){
    _pin = buttonPin;
    _longPress = t_long_press;
    pinMode(_pin, INPUT_PULLUP);
}

ButtonState ConfigButton::process(){
    uint32_t time = millis();

    if(_triggerTime + 50 < time) return; //Debounce

    bool btnPressed = !digitalRead(_pin);

    ButtonState state = ButtonState::Released;

    if(!_pressed && btnPressed){
        _triggerTime = time;
        _pressed = true;
        state = ButtonState::Pressed;
    }else if(_pressed){
        state = ButtonState::Pressed;
        uint32_t pressTime = time - _triggerTime;
        if (!btnPressed) {       
            _pressed = false;
            _triggerTime = time;
            if(pressTime < _longPress){
                state = ButtonState::Short_push;
            }    
            else{
                state = ButtonState::Long_push;
            }
        }else if(pressTime >= _longPress){
            state = ButtonState::Long_push;
        }
    }
    return state;
}