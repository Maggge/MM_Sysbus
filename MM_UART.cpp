#include "MM_UART.h"

MM_UART::MM_UART(Stream &serial) {
    MM_UART::_interface = &serial;
    MM_UART::_buf[0] = 0;
}

bool MM_UART::begin() {
    return true;
}

bool MM_UART::bufShift(uint8_t len) {
    if(_buf[0] == 0) return false;
    uint8_t i;

    while (len > 0) {
        for(i = (_buf[0]-1); i > 0; i--) {
            _buf[i]     = _buf[(i+1)];
            _buf[(i+1)] = 0;
        }
        len--;
        _buf[0]--;
    }

    return true;
}


bool MM_UART::bufShift() {
    if(_buf[0] == 0) return false;

    uint8_t i;

    do {
        for(i = (_buf[0]-1); i > 0; i--) {
            _buf[i]     = _buf[(i+1)];
            _buf[(i+1)] = 0;
        }
        _buf[0]--;
    }while(_buf[1] != 0x01 && _buf[0] > 0);
    return true;
}

bool MM_UART::Send(MM_MsgType msgType, uint16_t target, uint16_t source, uint8_t port, uint8_t len, uint8_t *data) {
    uint8_t tlen = 0;
    _interface->write(0x01);
    _interface->print(msgType,HEX);
    _interface->write(0x1F);
    _interface->print(target,HEX);
    _interface->write(0x1F);
    _interface->print(source,HEX);
    _interface->write(0x1F);
    if(port >= 0) {
        _interface->print(port,HEX);
    }else{
        //Arduino print internally casts to double :(
        _interface->print(F("FF"));
    }
    _interface->write(0x1F);
    _interface->print(len,HEX);

    _interface->write(0x02);
    if(len > 0) {
        for(tlen = 0; tlen < len; tlen++) {
            _interface->print(data[tlen], HEX);
            _interface->write(0x1F);
        }
    }
    _interface->write(0x04);
    _interface->println();
    return true;
}

bool MM_UART::Receive(MM_Packet &pkg) {
    uint8_t read;
    uint8_t state;
    uint8_t curwrite;
    bool retry = false;
    bool break2 = false;

    if(!_interface->available()) return false;

    while(_interface->available()) {
        read = _interface->read();

        if(read == 0x01) bufShift(_buf[0]);

        do {
            if(_buf[0] == 0) {  //No active RX, ignore everything until SOH
                if(read == 0x01) {
                    _buf[0] = 2;
                    _buf[1] = read;
                }
            }else{                
                _buf[_buf[0]] = read;

                _buf[0]++;
                if(_buf[(_buf[0]-1)] == 0x04 && _buf[0] >= 10) {
                    state = 0;
                    break2 = false;
                    for(read = 2; read <= _buf[0] && !break2; read++) {
                        switch(state) {
                            case 0:
                                pkg.meta.type = (MM_MsgType)HexToByte(_buf[read]);
                                state++;
                                break;
                            case 1:
                                if(_buf[read] != 0x1F) {
                                    retry = bufShift();
                                    break2 = true;
                                    break;
                                }else{
                                    state++;
                                }
                                break;
                            case 2:
                                if(_buf[read] == 0x1F) {
                                    state++;
                                }else{
                                    pkg.meta.target <<=4;
                                    pkg.meta.target |= HexToByte(_buf[read]);
                                }
                                break;
                            case 3:
                                if(_buf[read] == 0x1F) {
                                    state++;
                                    pkg.meta.port = 0;
                                }else{
                                    pkg.meta.source <<=4;
                                    pkg.meta.source |= HexToByte(_buf[read]);
                                }
                                break;
                            case 4:
                                if(_buf[read] == 0x1F) {
                                    state++;
                                }else{
                                    pkg.meta.port <<=4;
                                    pkg.meta.port |= HexToByte(_buf[read]);
                                }
                                break;
                            case 5:
                                pkg.len = HexToByte(_buf[read]);
                                state++;
                                break;
                            case 6:
                                if(_buf[read] != 0x02) {
                                    retry = bufShift();
                                    break2 = true;
                                    break;
                                }else{
                                    curwrite=0;
                                    state++;
                                }
                                break;
                            case 7:
                                if(_buf[read] == 0x1F) {
                                    curwrite++;
                                    pkg.data[curwrite] = 0;
                                }else if(_buf[read] == 0x04) {
                                    state++;
                                }else{
                                    pkg.data[curwrite] <<=4;
                                    pkg.data[curwrite] |= HexToByte(_buf[read]);
                                }
                                break;
                            case 8:
                                bufShift(read);
                                return true;
                        }
                    }
                }else if(_buf[0] > 35) {
                    retry = bufShift();
                }
            }
        }while(retry);
    }
    return false;
}

uint8_t MM_UART::HexToByte(uint8_t hex) {
    if(hex >= '0' && hex <= '9') return hex-'0';
    if(hex >= 'a' && hex <= 'f') return hex-'a'+10;
    if(hex >= 'A' && hex <= 'F') return hex-'A'+10;
    return 0;
}