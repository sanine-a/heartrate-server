#include "ConnectionManager.h"
#include <iostream>

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

ConnectionManager::ConnectionManager(ArduinoSerial& arduino,
                      unsigned int checkPingTime,
                      unsigned int errorPingTime)
    : arduino(arduino),
      checkPingTime(checkPingTime),
      errorPingTime(errorPingTime),
      pinging(false),
      connected(false)
{
    lastPingTime = std::chrono::steady_clock::now();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

bool ConnectionManager::connect(int maxAttempts)
{
    int attempts = 0;
    try {
        auto portlist = arduino.findMatchingPorts(METRO_MINI_VID, METRO_MINI_PID);
        if (portlist.size() > 0) {
            while(!arduino.openPort(portlist[0], 115200) && attempts < maxAttempts) {
                  std::cerr << "WARNING: handshake failed! retrying..." << std::endl;
                  attempts++;
                  arduino.closePort();
                  std::chrono::steady_clock::time_point lastTime = std::chrono::steady_clock::now();
                  while(std::chrono::steady_clock::now() - lastTime < std::chrono::seconds(5)) {}
            }
        }
        else {
            std::cerr << "ERROR: no arduino detected!" << std::endl;
            return false;
        }
    }
    catch (std::runtime_error error) {
        std::cerr << "FATAL: " << error.what() << std::endl;
        return false;
    }
    if (attempts == maxAttempts) {
        std::cerr << "ERROR: maximum handshake attempts exceeded!" << std::endl;
        return false;
    }
    connected = true;
    return true;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void ConnectionManager::update()
{
    std::chrono::duration<double> timeSincePing =
        std::chrono::duration_cast<std::chrono::duration<double>> (std::chrono::steady_clock::now() -
                                                                   lastPingTime);
    if (timeSincePing > errorPingTime) {
        std::cerr << "WARNING: arduino connection lost! attempting to reconnect..." << std::endl;
        connected = false;
        lastPingTime = std::chrono::steady_clock::now();
        pinging = false;
        arduino.closePort();
        while(!connect(10));
    }
    else if (timeSincePing > checkPingTime && !pinging) {
        arduino.send("wake-arduino", "1");
        pinging = true;
    }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    
    
void ConnectionManager::pingReceived()
{
    pinging = false;
    lastPingTime = std::chrono::steady_clock::now();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

bool ConnectionManager::isConnected()
{
    return connected;
}
