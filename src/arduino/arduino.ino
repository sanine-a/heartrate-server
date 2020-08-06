#include "SerialController.hpp"
#include "Button.h"

#define BAUD_RATE 115200

SerialController serial;
Button leadsOffDetect;

#define SIGNAL_PIN A0
#define LOD_PIN 2

void serialParse(char* key, char* value);

void setup()
{
    serial.setup(BAUD_RATE, serialParse);
    leadsOffDetect.setup(LOD_PIN, [](int state) { serial.sendMessage("leads-off", 1-state); });
}

void loop() {
    serial.update();
    leadsOffDetect.update();
    if (leadsOffDetect.state == false)
        serial.sendMessage("signal", analogRead(SIGNAL_PIN));
}

void serialParse(char* key, char* value)
{
    if (strcmp(key, "wake-arduino") == 0)
        serial.sendMessage("arduino-ready", "1");
}
