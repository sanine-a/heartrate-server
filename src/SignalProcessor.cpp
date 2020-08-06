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
    static int i = 0;
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

    std::cout << i << " "
              << datapoint << " "
              << signalLowPass << " "
              << signal << " "
              << positiveSignal << " "
              << diff.getDerivative() << " "
              << derivativeLowPass << " "
              << diffMax.getMaximum() << " "
              << diffMaxAvg.getAverage() << std::endl;
    i++;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void SignalProcessor::setTriggerCallback(void (*callback)())
{
    this->callback = callback;
}
            
        

