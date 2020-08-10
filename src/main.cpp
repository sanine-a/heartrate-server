#include <iostream>
#include <SFML/Audio.hpp>
#include <algorithm>
#include <fstream>
#include <sstream>

#include "smmServer.hpp"
#include "ArduinoSerial.h"
#include "SignalProcessor.h"
#include "ConnectionManager.h"
#include "tinydir.h"

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
bool leadsOff = true;

std::vector<std::string> getLogFiles()
{
    tinydir_dir dir;
    std::vector<std::string> fileList;
    tinydir_open(&dir, "./logs");
    while (dir.has_next) {
        tinydir_file file;
        tinydir_readfile(&dir, &file);

        if (!file.is_dir)
            fileList.push_back(file.name);
        
        tinydir_next(&dir);
    }

    tinydir_close(&dir);

    std::sort(fileList.begin(), fileList.end(),
              [](std::string a, std::string b) { return b.compare(a) < 0; });
    
    return fileList;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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
    else if (key == "leads-off") {
        if (value == "1") {
            leadsOff = true;
            ecgSignal->leadsOff();
        }
        else {
            leadsOff = false;
            ecgSignal->leadsOn();
        }
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

void getLogList(httpMessage message,
                void* d)
{
    auto fileList = getLogFiles();
    std::string fileListData = "[";
    for (auto fileName = fileList.begin(); fileName != fileList.end(); fileName++)
        fileListData += "\"" + *fileName + "\",";
    if (fileListData.length() > 1)
        fileListData.pop_back();
    fileListData += "]";
    message.replyHttpContent("text/plain", fileListData);
}

void getLogFile(httpMessage message,
                void* d)
{
    try {
        std::string fileName = message.getHttpVariable("file");
        std::ifstream file("./logs/" + fileName);
        std::stringstream buffer;
        buffer << file.rdbuf();
        message.replyHttpContent("text/plain", buffer.str());
    }
    catch(...) {
        message.replyHttpError(500, "An error occurred");
    }
}

void checkStatus(httpMessage message,
                 void* d)
{
    if (!connManager->isConnected()) {
        message.replyHttpContent("text/plain", "ADC not connected");
    }
    else if (leadsOff) {
        message.replyHttpContent("text/plain", "ADC connected, leads off");
    }
    else {
        message.replyHttpContent("text/plain", "ADC connected, leads on");
    }
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
    server.addPostCallback("checkStatus", checkStatus);
    server.addPostCallback("getLogList", getLogList);
    server.addPostCallback("getLogFile", getLogFile);
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
