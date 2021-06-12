#include <MM_Sysbus.h>

MM_Sysbus sysbus(0); //Controller with node-ID 0 - Gateway-Mode: faster routing between interfaces but no Modules or Hooks supported

MM_CAN can(10, CAN_125KBPS, MCP_8MHZ, 2); //Can-Bus Interface

MM_UART uart(Serial);   //Serial Interface

void setup() {
  Serial.begin(115200); //Initialize Serial

  sysbus.attachBus(&uart); //Attach Serial Interface to the Controller
  sysbus.attachBus(&can);  //Attach Canbus Interface to the Controller
}

void loop() {
  sysbus.loop();    //Main loop of the controller
}