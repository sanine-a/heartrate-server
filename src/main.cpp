#include <iostream>
#include <SFML/Audio.hpp>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <mutex>

#include "smmServer.hpp"
#include "ArduinoSerial.h"
#include "SignalProcessor.h"
#include "ConnectionManager.h"
#include "tinydir.h"
#include "json.hpp"

extern "C" {
    #include "b64/base64.h"
}

#define MAX_DATA_POINTS 8192

struct SignalProcessorParams {
    double signal_lowpass = 0.1;
    double derivative_lowpass = 0.2;
    unsigned int n_avg_samples = 20;
    unsigned int n_max_samples = 300;
    unsigned int max_persistence_time = 50;
    double max_scale = 3;
} params;
bool paramsChanged = false;
std::mutex paramsMutex;

ArduinoSerial* arduino;
ConnectionManager* connManager;
SignalProcessor* ecgSignal;
bool leadsOff = true;

sf::SoundBuffer heartbeatSoundBuffer;
sf::Sound heartbeat;
std::mutex soundMutex;

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
    }
    else if (key == "leads-off") {
        if (value == "1") {
            leadsOff = true;
            ecgSignal->leadsOff();
            if (paramsChanged) {
                paramsMutex.lock();
                delete ecgSignal;
                ecgSignal = new SignalProcessor(params.signal_lowpass,
                                                params.derivative_lowpass,
                                                params.n_avg_samples,
                                                params.n_max_samples,
                                                params.max_persistence_time,
                                                params.max_scale);
                paramsChanged = false;
                paramsMutex.unlock();
            }
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

void uploadAudio(httpMessage message,
                 void* d)
{
    try {
        std::string audioBase64 = message.getHttpVariable("audio");

        unsigned long audioSize = 8*audioBase64.length() + 1;
        unsigned char* audioBinary = (unsigned char*) malloc(audioSize * sizeof(unsigned char));
        int resultSize = b64_decode((const unsigned char*) audioBase64.c_str(),
                                    audioBase64.length(),
                                    audioBinary);

        std::ofstream audioFile("heartbeat.wav", std::ios::binary);
        audioFile.write((const char*) audioBinary, resultSize);
        audioFile.close();

        free(audioBinary);

        if(heartbeatSoundBuffer.loadFromFile("heartbeat.wav")) {
            soundMutex.lock();
            heartbeat.setBuffer(heartbeatSoundBuffer);
            soundMutex.unlock();
        }

        std::cout << "heartbeat.wav updated" << std::endl;
        
        message.replyHttpOk();
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

void getParams(httpMessage message,
               void* d)
{
    nlohmann::json j;

    paramsMutex.lock();
    j["signal_lowpass"]              = params.signal_lowpass;
    j["derivative_lowpass"]          = params.derivative_lowpass;
    j["n_avg_samples"]               = params.n_avg_samples;
    j["n_max_samples"]               = params.n_max_samples;          
    j["max_persistence_time"]        = params.max_persistence_time;   
    j["max_scale"]                   = params.max_scale;              
    paramsMutex.unlock();

    std::string paramsString = j.dump();
    message.replyHttpContent("text/plain", paramsString);
}    

void updateParamsFromFile(std::string filename)
{
    std::ifstream file(filename);
    nlohmann::json j;
    file >> j;
    file.close();

    paramsMutex.lock();
    
    params.signal_lowpass = j["signal_lowpass"].get<double>();
    params.derivative_lowpass = j["derivative_lowpass"].get<double>();
    params.n_avg_samples = j["n_avg_samples"].get<int>();
    params.n_max_samples = j["n_max_samples"].get<int>();
    params.max_persistence_time = j["max_persistence_time"].get<int>();
    params.max_scale = j["max_scale"].get<int>();

    paramsMutex.unlock();
}

void updateParams(httpMessage message,
                  void* d)
{
    try {
        double signal_lowpass = std::stof(message.getHttpVariable("signal_lowpass"));
        double derivative_lowpass = std::stof(message.getHttpVariable("derivative_lowpass"));
        unsigned int n_avg_samples = std::stoi(message.getHttpVariable("n_avg_samples"));
        unsigned int n_max_samples = std::stoi(message.getHttpVariable("n_max_samples"));
        unsigned int max_persistence_time = std::stoi(message.getHttpVariable("max_persistence_time"));
        double max_scale = std::stof(message.getHttpVariable("max_scale"));

        nlohmann::json j;
        j["signal_lowpass"]              = signal_lowpass;
        j["derivative_lowpass"]          = derivative_lowpass;
        j["n_avg_samples"]               = n_avg_samples;
        j["n_max_samples"]               = n_max_samples;          
        j["max_persistence_time"]        = max_persistence_time;   
        j["max_scale"]                   = max_scale;              

        std::ofstream file("settings.json");
        file << j << std::endl;
        file.close();

        updateParamsFromFile("settings.json");
        
        paramsChanged = true;

        message.replyHttpOk();
    }
    catch(...) {
        message.replyHttpError(500, "An error occurred");
    }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

int main()
{
    updateParamsFromFile("settings.json");
    
    arduino = new ArduinoSerial();
    connManager = new ConnectionManager(*arduino, 5, 6);
    ecgSignal = new SignalProcessor(params.signal_lowpass,
                                    params.derivative_lowpass,
                                    params.n_avg_samples,
                                    params.n_max_samples,
                                    params.max_persistence_time,
                                    params.max_scale);

    if (!heartbeatSoundBuffer.loadFromFile("heartbeat.wav")) {
        std::cerr << "ERROR: could not find 'heartbeat.wav'!" << std::endl;
        return 1;
    }
    heartbeat.setBuffer(heartbeatSoundBuffer);

    ecgSignal->setTriggerCallback([]() {
            soundMutex.lock();
            heartbeat.play();
            soundMutex.unlock();
        });

    arduino->setDataCallback(processMessage);

    if (!connManager->connect(10))
        return 1;

    smmServer server("8000", "./web_root", NULL);
    server.addPostCallback("getData", getData);
    server.addPostCallback("checkStatus", checkStatus);
    server.addPostCallback("getLogList", getLogList);
    server.addPostCallback("getLogFile", getLogFile);
    server.addPostCallback("uploadAudio", uploadAudio);
    server.addPostCallback("updateParams", updateParams);
    server.addPostCallback("getParams", getParams);
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
