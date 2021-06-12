#include "MM_CAN.h"

volatile bool MM_CANIntReq = false;

void MM_CANInt() {
    MM_CANIntReq=true;
}

MM_CAN::MM_CAN(uint8_t cs, uint8_t speed, uint8_t clockspd, uint8_t interrupt) : 
_interface(cs) {
    pinMode(interrupt, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(interrupt), MM_CANInt, FALLING);
    _csPin = cs;
    _intPin = interrupt;
    _speed = speed;
    _clockspd = clockspd;
    //Begin doesn't work here!
}

bool MM_CAN::begin() {
    lastErr = _interface.begin(MCP_ANY, _speed, _clockspd);
    _interface.setMode(MCP_NORMAL);
    return (lastErr == 0);
}

MM_Meta MM_CAN::CanAddrParse(uint32_t canAddr) {
    MM_Meta temp;

    #ifdef MM_DEBUG
        Serial.print("Address: ");
        Serial.println(canAddr);
    #endif

    temp.target = 0;
    temp.source = 0;
    temp.port = 0;

    temp.type = (MM_MsgType)((canAddr >> 27) & 0x03);
    temp.source = (canAddr & 0x7FF);
    temp.target = ((canAddr >> 11) & 0xFFFF);

    if(temp.type == Unicast || temp.type == Broadcast) {
        temp.port = ((canAddr >> 23) & 0x1F);
        temp.target &= 0x7FF;
    }

    return temp;
}

uint32_t MM_CAN::CanAddrAssemble(MM_Meta meta) {
    return CanAddrAssemble(meta.type, meta.target, meta.source, meta.port);
}

uint32_t MM_CAN::CanAddrAssemble(MM_MsgType msgType, uint16_t target, uint16_t source) {
    return CanAddrAssemble(msgType, target, source, 0);
}

uint32_t MM_CAN::CanAddrAssemble(MM_MsgType msgType, uint16_t target, uint16_t source, uint8_t port) {
        uint32_t addr = 0x80000000;

        if(msgType > 0x04) return 0;
        addr |= ((uint32_t)msgType << 27);

        if(msgType == Unicast || msgType == Broadcast || msgType == Streaming) {
            if(target > 0x7FF) return 0;
            if(port > 0x1F) return 0;

            addr |= ((uint32_t)port << 23);
        }else{
            if(target > 0xFFFF) return 0;
        }

        addr |= ((uint32_t)target << 11);

        if(source > 0x7FF) return 0;
        addr |= source;

        #ifdef MM_DEBUG
            Serial.print("Address: ");
            Serial.println(addr);
        #endif

        return addr;
}

bool MM_CAN::Send(MM_MsgType msgType, uint16_t target, uint16_t source, uint8_t port, uint8_t len, uint8_t *data) {
    uint32_t addr = CanAddrAssemble(msgType, target, source, port);
    if(addr == 0) return false;

    lastErr = _interface.sendMsgBuf(addr, 1, len, data);
    if(lastErr != CAN_OK) return false;
    return true;
}

bool MM_CAN::Receive(MM_Packet &pkg) {

    uint32_t rxId;
    uint8_t ext = 1;
    uint8_t len = 0;
    uint8_t rxBuf[8];

    if(_interface.checkReceive() != CAN_MSGAVAIL) return false;

    uint8_t state = _interface.readMsgBuf(&rxId, &len, rxBuf);


    if(state != CAN_OK) return false;

    pkg.meta = CanAddrParse(rxId);
    pkg.len = len;

    for(uint8_t i=0; i<len; i++) pkg.data[i] = rxBuf[i];

    return true;
}