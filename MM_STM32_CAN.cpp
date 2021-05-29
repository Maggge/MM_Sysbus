#include "MM_STM32_CAN.h"

MM_STM32_CAN::MM_STM32_CAN(BitRate bitRate, BusType busType){
    _bitRate = bitRate;
    _busType = busType;
}

bool MM_STM32_CAN::begin() {
    _can.begin(EXT_ID_LEN, _bitRate, _busType); // 29b IDs, set bit rate, no transceiver chip, portA pins 11,12
    _can.filterMask16Init(0, 0, 0x7ff, 0, 0);
    return true;
}

bool MM_STM32_CAN::Send(MM_MsgType msgType, uint16_t target, uint16_t source, uint8_t port, uint8_t len, uint8_t *data){
    uint32_t addr = MM_CAN::CanAddrAssemble(msgType, target, source, port);
    if(addr == 0) return false;

    return _can.transmit(addr, data, len);
}

bool MM_STM32_CAN::Receive(MM_Packet &pkg){
    int rxId;
    int fltIdx;
    uint8_t ext = 1;
    uint8_t rxBuf[8];

    int len = _can.receive(rxId, fltIdx, rxBuf);
    

    if(len > -1){
        pkg.meta = MM_CAN::CanAddrParse(rxId);
        pkg.len = len;
        for(uint8_t i=0; i<len; i++) pkg.data[i] = rxBuf[i];
        return true;
    } 
    return false;
    
}