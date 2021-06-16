#ifndef __MM_BasicIO__
#define __MM_BasicIO__

#include <Arduino.h>
#include "MM_Module.h"
#include "MM_Sysbus.h"

class MM_Digital_Out : public MM_Module{
private:    
        enum ON_MODE{
        Off,
        LastState,
        On,
    };

    typedef struct cfg
    {
        bool state;
        bool inverted;
        ON_MODE powerBack;
    };

    /**
     * pin of the output
     */
    uint8_t _pin;

    /**
     * config of the module, it will be stored in the EEPROM
     */
    cfg _config;

public:
    /**
     * Basic digital output
     * @param pin of the output
     * @param port of the module
     * @param inverted - if true: Off=HIGH, if false: Off=LOW
     */
    MM_Digital_Out(uint8_t pin, uint8_t port, bool inverted);
    void begin(bool useEEPROM, uint8_t cfgId);
    bool process(MM_Packet &pkg);
    bool loop();
    bool broadcastState();
    void switchOutput(bool power);
};

#endif