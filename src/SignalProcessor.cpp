#include "SignalProcessor.h"
#include "date.h"
#include <fstream>

SignalProcessor::SignalProcessor(double signalLowPass,
                                 double derivativeLowPass,
                                 unsigned int numAvgSamples,
                                 unsigned int numMaxAvgSamples,
                                 unsigned int maxPersistenceTime,
                                 double maxScale)
    : avg(numAvgSamples),
      diffMaxAvg(numMaxAvgSamples),
      diff(),
      diffMax(maxPersistenceTime),
      signalLowPassWeight(signalLowPass),
      signalLowPass(0),
      derivativeLowPassWeight(derivativeLowPass),
      derivativeLowPass(0),
      maxScale(maxScale),
      leadsAreOff(false),
      triggered(false),
      callback(nullptr) {}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void SignalProcessor::leadsOn()
{
    leadsAreOff = false;
}

void SignalProcessor::leadsOff()
{
    leadsAreOff = true;
    std::string datetime = date::format("%Y-%m-%d_%H:%M:%S", std::chrono::system_clock::now());
    std::string logFileName = "logs/" + datetime + ".dat";

    std::ofstream logFile;
    logFile.open(logFileName);
    logFile <<
        "# raw signal, lowpass filtered signal, derivative of positive part, lowpass filtered derivative, rolling maxiumum average of derivative"
            << std::endl;
    for (int i=0; i<rawSignal.size(); i++) {
        logFile << i << " "
                << rawSignal[i] << " "
                << filteredSignal[i] << " "
                << signalDerivative[i] << " "
                << filteredDerivative[i] << " "
                << derivativeMaxAverage[i] << std::endl;
    }
    logFile.close();

    rawSignal.clear();
    filteredSignal.clear();
    signalDerivative.clear();
    filteredDerivative.clear();
    derivativeMaxAverage.clear();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void SignalProcessor::addDataPoint(double datapoint)
{
    signalLowPass =
        (1-signalLowPassWeight)*signalLowPass +
        signalLowPassWeight * datapoint;
    
    avg.addDataPoint(signalLowPass);

    double signal = signalLowPass - avg.getAverage(); // center about zero
    double positiveSignal = signal > 0 ? signal : 0;

    diff.addDataPoint(positiveSignal);
    derivativeLowPass =
        (1-derivativeLowPassWeight)*derivativeLowPass +
        derivativeLowPassWeight * diff.getDerivative();

    diffMax.addDataPoint(derivativeLowPass);
    diffMaxAvg.addDataPoint(diffMax.getMaximum());

    if (derivativeLowPass > maxScale*diffMaxAvg.getAverage()) {
        if (!triggered) {
            triggered = true;
            if (callback != nullptr)
                callback();
        }
    }
    else
        triggered = false;

    protection.lock();
    rawSignal.push_back(datapoint);
    filteredSignal.push_back(signal);
    signalDerivative.push_back(diff.getDerivative());
    filteredDerivative.push_back(derivativeLowPass);
    derivativeMaxAverage.push_back(diffMaxAvg.getAverage());
    protection.unlock();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void SignalProcessor::setTriggerCallback(void (*callback)())
{
    this->callback = callback;
}
            
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

std::string SignalProcessor::getDataString(unsigned long lastIndex,
                                           unsigned int signalSelect = 0)
{
    std::string dataString = "[";
    std::vector<double>* dataset;
    switch(signalSelect) {
    case 0:
        dataset = &rawSignal;
        break;

    case 1:
        dataset = &filteredSignal;
        break;

    case 3:
        dataset = &signalDerivative;
        break;

    case 4:
        dataset = &filteredDerivative;
        break;

    case 5:
        dataset = &derivativeMaxAverage;
        break;

    default:
        return "[]";
    }

    protection.lock();
    if (lastIndex > dataset->size()) {
        protection.unlock();
        return "[]";
    }

    for (auto i = dataset->begin()+lastIndex; i != dataset->end(); i++) {
        dataString += std::to_string(*i);
        dataString += ",";
    }

    if (dataString.length() > 1)
        dataString.pop_back();

    dataString += "]";
    protection.unlock();

    return dataString;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
