#include <iostream>
#include <chrono>
#include <deque>
#include <SFML/Audio.hpp>

#include "smmServer.hpp"
#include "ArduinoSerial.h"
#include "SignalProcessor.h"

#define MAX_DATA_POINTS 8192

#define SIGNAL_LOWPASS 0.1
#define DERIVATIVE_LOWPASS 0.2
#define N_AVG_SAMPLES 20
#define N_MAX_SAMPLES 300
#define MAX_PERSISTENCE_TIME 50
#define MAX_SCALE 3

const std::string AUDIO_DIRECTORY = "./audio/";
SignalProcessor* ecgSignal;
sf::Sound sound;
bool pinging;
int numNewDataPoints = 0;
std::chrono::steady_clock::time_point lastPingTime;
std::chrono::duration<double> checkPingTime(5);
std::chrono::duration<double> errorPingTime(6);
sf::SoundBuffer buffer;

void processMessage(std::string key, std::string value)
{
    //std::cout << key << ":" << value << std::endl;
    if (key == "arduino-ready") {
        pinging = false;
        lastPingTime = std::chrono::steady_clock::now();
    }
    else if (key == "signal") {
        int v = stoi(value);
        ecgSignal->addDataPoint(v);
        numNewDataPoints++;
    }
}
    
void getData(httpMessage message,
             void* d)
{}
/*
    int startIndex = data.size() - numNewDataPoints;
    numNewDataPoints = 0;
    std::string dataString = "[";
    for (auto i = data.begin()+startIndex; i != data.end(); i++) {
        dataString += std::to_string(*i);
        dataString += ',';
    }
    if (dataString.length() > 1)
        dataString.pop_back();
    dataString += "]";
    message.replyHttpContent("text/plain", dataString);
    }*/

bool connectArduino(ArduinoSerial& arduino, int maxAttempts = 10)
{
    int attempts = 0;
    try {
        auto portlist = arduino.findMatchingPorts(METRO_MINI_VID, METRO_MINI_PID);
        if (portlist.size() > 0) {
            while(!arduino.openPort(portlist[0], 115200) && attempts < maxAttempts) {
                  std::cerr << "WARNING: handshake failed! retrying..." << std::endl;
                  attempts++;
                  arduino.closePort();
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
    return true;
}


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

int main()
{
    ecgSignal = new SignalProcessor(SIGNAL_LOWPASS,
                                    DERIVATIVE_LOWPASS,
                                    N_AVG_SAMPLES,
                                    N_MAX_SAMPLES,
                                    MAX_PERSISTENCE_TIME,
                                    MAX_SCALE);
    
    if (!buffer.loadFromFile("heartbeat.wav")) {
        std::cerr << "ERROR: could not find 'heartbeat.wav'!" << std::endl;
        return 1;
    }
    sound.setBuffer(buffer);

    ecgSignal->setTriggerCallback([]() { sound.play(); });
        
    ArduinoSerial arduino;
    pinging = false;
    arduino.setDataCallback(processMessage);
    if (connectArduino(arduino)) {
        lastPingTime = std::chrono::steady_clock::now();
    }
    else
        return 1;

    smmServer server("8000", "./web_root", NULL);
    server.addPostCallback("getData", getData);
    server.launch();

    std::cout << "Launched server on port 8000" << std::endl;

    while(server.isRunning()) {
        std::chrono::duration<double> timeSincePing =
            std::chrono::duration_cast<std::chrono::duration<double>> (std::chrono::steady_clock::now() -
                                                                       lastPingTime);
        if (timeSincePing > errorPingTime) {
            std::cerr << "WARNING: arduino connection lost! attempting to reconnect..." << std::endl;
            lastPingTime = std::chrono::steady_clock::now();
            pinging = false;
            arduino.closePort();
            connectArduino(arduino);
        }
        else if (timeSincePing > checkPingTime && !pinging) {
            arduino.send("wake-arduino", "1");
            pinging = true;
        }
        arduino.update();
    }

    delete ecgSignal;

    return 0;
}
