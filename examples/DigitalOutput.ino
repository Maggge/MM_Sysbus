#include <MM_Sysbus.h>

MM_Sysbus sysbus(90, 0); //Controller initaialized with Address 90 and EEPROM-StartAddress 0

MM_CAN can(10, CAN_125KBPS, MCP_8MHZ, 2);

MM_Digital_Out relay(7, 0, true); //Define a digital output with pin-number 7, port 0, and inverted(ON=LOW);

void setup() {
    sysbus.attachBus(&can); //Attach the can-bus to the controller
    sysbus.attachModule(&relay); //Attach the module to the controller
}

void loop() {
  sysbus.loop();
}