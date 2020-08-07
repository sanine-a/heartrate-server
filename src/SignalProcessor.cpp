#include "SignalProcessor.h"
#include <iostream>

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

    rawSignal.push_back(datapoint);
    filteredSignal.push_back(signal);
    signalDerivative.push_back(diff.getDerivative());
    filteredDerivative.push_back(derivativeLowPass);
    derivativeMaxAverage.push_back(diffMaxAvg.getAverage());
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

    if (lastIndex > dataset->size())
        return "[]";

    for (auto i = dataset->begin()+lastIndex; i != dataset->end(); i++) {
        dataString += std::to_string(*i);
        dataString += ",";
    }

    if (dataString.length() > 1)
        dataString.pop_back();

    dataString += "]";

    std::cout << dataString << std::endl;
    
    return dataString;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
