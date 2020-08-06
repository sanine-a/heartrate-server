#ifndef ARDUINO_CONNECTION_MANAGER_H
#define ARDUINO_CONNECTION_MANAGER_H

#include "ArduinoSerial.h"
#include <chrono>

class ConnectionManager
{
 public:
    ConnectionManager(ArduinoSerial& arduino,
                      unsigned int checkPingTime,
                      unsigned int errorPingTime);
    bool connect(int maxAttempts);
    void update();
    void pingReceived();

 private:
    ArduinoSerial& arduino;
    std::chrono::steady_clock::time_point lastPingTime;
    std::chrono::duration<double> checkPingTime;
    std::chrono::duration<double> errorPingTime;
    bool pinging;
};

#endif
