#include "MM_Sysbus.h"

MM_Sysbus::MM_Sysbus(uint16_t nodeID) {
    _nodeID = nodeID;
    _useEEPROM = false;
    _initialized = true;
    _firstboot = true;
     #ifdef MM_DEBUG
        Serial.print("Node initailized with ID: ");
        Serial.println(_nodeID);
    #endif
}

MM_Sysbus::MM_Sysbus(uint16_t nodeID, uint16_t EEPROMstart){
    _nodeID = nodeID;
    _useEEPROM = true;
    _EEPROMaddr = EEPROMstart;

    uint8_t cfg;
    EEPROM.get(_EEPROMaddr, cfg);
    if (cfg == 99) {
        EEPROM.get(_EEPROMaddr + 1, _nodeID);
        _firstboot = false;
    }
    else{
        EEPROM.update(_EEPROMaddr, 99);
        EEPROM.update(_EEPROMaddr + 1, _nodeID);
        _firstboot = true;
    }
    _initialized = true;
    #ifdef MM_DEBUG
        Serial.print("Node initailized with ID: ");
        Serial.println(_nodeID);
    #endif
}

MM_Sysbus::MM_Sysbus(uint8_t cfgButton, uint8_t statusLED, uint16_t EEPROMstart){
    _useEEPROM = true;
    _EEPROMaddr = EEPROMstart;

    _cfgBtn = cfgButton;
    _statusLED = statusLED;
    pinMode(_statusLED, OUTPUT);
    pinMode(_cfgBtn, INPUT_PULLUP);

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
    if(nodeID < 0 || nodeID > 2047){
        return false;
    }
    _nodeID = nodeID;
    _initialized = true;
    if(_useEEPROM){
        EEPROM.update(_EEPROMaddr, (uint8_t)99);
        EEPROM.put(_EEPROMaddr + 1, _nodeID);
    }
    #ifdef MM_DEBUG
        Serial.print("Set new Node ID: ");
        Serial.println(_nodeID);
    #endif
    return true;
}

uint16_t MM_Sysbus::nodeID(){
    return _nodeID;
}

bool MM_Sysbus::attachBus(MM_Interface* bus){
    for(int i = 0; i < MM_BUSNUM; i++){
        if(_interfaces[i] == bus){
            #ifdef MM_DEBUG
                Serial.println("Bus already attached");
            #endif
            return false;
        }
    }
    for(int busId = 0; busId < MM_BUSNUM; busId++){
        if(_interfaces[busId] == NULL){
            _interfaces[busId] = bus;

            if (bus->begin()) {
                //Boot message
                uint8_t data[1] = { NODE_BOOT };
                bus->Send(Broadcast, 0x00, _nodeID, 0, sizeof(data), data);

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
    for(int i = 0; i < MM_BUSNUM; i++){
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

    for (signed char busId = 0; busId < MM_BUSNUM; busId++) {
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
        Process(pkg);
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

    for (signed char busId = 0; busId < MM_BUSNUM; busId++) {
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
                if(_initialized){
                    Process(pkg);
                }

                if (routing) {
                    //Resend to every attached interface except the one we received it on (pkg.meta.busId)
                    Send(pkg);
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

    //Internal logic
    if (pkg.len >= 1) {
        switch (pkg.data[0]) {
        case RESET_NODE:
            if (pkg.meta.type != Unicast || pkg.meta.target != _nodeID) break;       
            reset();
            break;
        case NODE_PING:
            if (pkg.meta.type != Unicast || pkg.meta.target != _nodeID) break;
            data[0] = NODE_PONG;
            Send(Unicast, pkg.meta.source, _nodeID, pkg.meta.port, 1, data, -1);
            break;
        case NODE_ID:
            if (pkg.meta.type != Unicast || pkg.meta.target != _nodeID || pkg.len < 3) break;
            int id = int(data[1] << 8) + int(data[2]);
            setNodeId(id);
            break;
        }
    }

    //attached modules
    if (((pkg.meta.type == Unicast || pkg.meta.type == Streaming) && pkg.meta.target == _nodeID) || pkg.meta.type == Multicast) {
        for (int i = 0; i < MM_MODULNUM; i++) {
            if (_modules[i] != NULL) {
                _modules[i]->process(pkg);
            }
        }
    }

    //attached hooks
    for (i = 0; i < MM_HOOKNUM; i++) {
        if (_hooks[i].execute != NULL) {
            if (
                (_hooks[i].type == pkg.meta.type) &&
                (_hooks[i].target == 0 || _hooks[i].target == pkg.meta.target) &&
                (_hooks[i].port == -1  || _hooks[i].port == pkg.meta.port) &&
                (_hooks[i].cmd == ALL_CMDS  || (pkg.len > 0 && _hooks[i].cmd == pkg.data[0]))){
                _hooks[i].execute(pkg);
            }
        }
    }
}

bool MM_Sysbus::attachModule(MM_Module *module, uint8_t cfgId){
    for(int i = 0; i < MM_MODULNUM; i++){
        if(_modules[i] == module){
            #ifdef MM_DEBUG
                Serial.println("Module already attached");
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
    for(int i = 0; i < MM_MODULNUM; i++){
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
    for(int i = 0; i < MM_MODULNUM; i++){
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

bool MM_Sysbus::hookAttach(MM_MsgType msgType, uint16_t target, uint8_t port, MM_CMD cmd, void (*function)(MM_Packet&)) {
    for (byte i = 0; i < MM_HOOKNUM; i++) {
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

    processCfgButton();
    
    if (_cfgBtnPressed && _cfgBtnTriggerTime + 5000 <= millis()) {
        initialization();
    }
    if (!_initialized) {
        digitalWrite(_statusLED, HIGH);
        return pkg;
    }    

    //Modules loop
    for (int i = 0; i < MM_MODULNUM; i++) {
        if (_modules[i] != NULL) {
            _modules[i]->loop();
        }
    }

    return pkg;
}

void MM_Sysbus::processCfgButton(){
    if(_cfgBtnTriggerTime + 50 < millis()) return; //Debounce
    if(!_cfgBtnPressed && digitalRead(_cfgBtn) == LOW){
        _cfgBtnTriggerTime = millis();
        _cfgBtnPressed = true;

    }else if (_cfgBtnPressed && digitalRead(_cfgBtn) == HIGH) {
        _cfgBtnPressed = false;
        _cfgBtnTriggerTime = millis();
    }
}

void MM_Sysbus::initialization() {
    MM_Packet pkg;
    uint32_t exitTime = millis() + 60000;
    while (exitTime >= millis()) {

        processCfgButton();

        if (_cfgBtnPressed && _cfgBtnTriggerTime + 10000 <= millis()) {
            reset();
        }

        if (millis() % 1000 > 500) {
            digitalWrite(_statusLED, HIGH);
        }
        else {
            digitalWrite(_statusLED, LOW);
        }

        if (Receive(pkg) && pkg.data[0] == NODE_ID && pkg.len == 1) {
            setNodeId(pkg.meta.target); 
            Send(Broadcast, pkg.meta.source, pkg.len, pkg.data);
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
    return (cfgID * REGISTER_PER_MODUL) + _EEPROMaddr + 3;
}

void MM_Sysbus::reset(){
    for (int i = 0; i < 10; i++){
        delay(50);
        digitalWrite(_statusLED, HIGH);
        delay(50);
        digitalWrite(_statusLED, LOW);
    }
    for (int i = 0 ; i < EEPROM.length() ; i++) {
        EEPROM.write(i, 0);
    }
    delay(100);
    wdt_enable(WDTO_15MS);
    while (true);  
}