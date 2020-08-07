#include <iostream>
#include <chrono>
#include <deque>
#include <SFML/Audio.hpp>

#include "smmServer.hpp"
#include "ArduinoSerial.h"
#include "SignalProcessor.h"
#include "ConnectionManager.h"

#define MAX_DATA_POINTS 8192

#define SIGNAL_LOWPASS 0.1
#define DERIVATIVE_LOWPASS 0.2
#define N_AVG_SAMPLES 20
#define N_MAX_SAMPLES 300
#define MAX_PERSISTENCE_TIME 50
#define MAX_SCALE 3

ArduinoSerial* arduino;
ConnectionManager* connManager;
SignalProcessor* ecgSignal;
sf::Sound heartbeat;
int numNewDataPoints = 0;

void processMessage(std::string key, std::string value)
{
    if (key == "arduino-ready") {
        connManager->pingReceived();
    }
    else if (key == "signal") {
        int v = stoi(value);
        ecgSignal->addDataPoint(v);
        numNewDataPoints++;
    }
}
    
void getData(httpMessage message,
             void* d)
{
    int lastIndex, signalSelect;
    try {
        std::string indexString =  message.getHttpVariable("index");
        std::string signalString = message.getHttpVariable("signal");
        lastIndex = std::stoi(indexString);
        signalSelect = std::stoi(signalString);
    }
    catch(std::exception error) {
        std::cerr << "error parsing POST request: " << error.what() << std::endl;
        message.replyHttpContent("text/plain", "[]");
    }
    message.replyHttpContent("text/plain", ecgSignal->getDataString(lastIndex, signalSelect));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

int main()
{
    arduino = new ArduinoSerial();
    connManager = new ConnectionManager(*arduino, 5, 6);
    ecgSignal = new SignalProcessor(SIGNAL_LOWPASS,
                                    DERIVATIVE_LOWPASS,
                                    N_AVG_SAMPLES,
                                    N_MAX_SAMPLES,
                                    MAX_PERSISTENCE_TIME,
                                    MAX_SCALE);
    
    sf::SoundBuffer buffer;
    if (!buffer.loadFromFile("heartbeat.wav")) {
        std::cerr << "ERROR: could not find 'heartbeat.wav'!" << std::endl;
        return 1;
    }
    heartbeat.setBuffer(buffer);

    ecgSignal->setTriggerCallback([]() { heartbeat.play(); });

    arduino->setDataCallback(processMessage);

    if (!connManager->connect(10))
        return 1;

    smmServer server("8000", "./web_root", NULL);
    server.addPostCallback("getData", getData);
    server.launch();

    std::cout << "Launched server on port 8000" << std::endl;

    while(server.isRunning()) {
        connManager->update();
        arduino->update();
    }

    delete ecgSignal;
    delete connManager;
    delete arduino;

    return 0;
}
