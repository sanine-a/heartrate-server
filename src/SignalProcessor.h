#ifndef ECG_SIGNAL_PROCESSOR_H
#define ECG_SIGNAL_PROCESSOR_H

#include <vector>
#include <deque>
#include <string>
#include <mutex>

class Averager
{
 public:
    Averager(unsigned int numSamples);
    void addDataPoint(double datapoint);
    double getAverage();

 private:
    int numSamples;
    double total;
    std::deque<double> samples;
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class Differentiator
{
 public:
    Differentiator();
    void addDataPoint(double datapoint);
    double getDerivative();

 private:
    double datapoint0, datapoint1;
    double derivative;
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class RollingMaximum
{
 public:
    RollingMaximum(unsigned int maximumPersistenceTime);
    void addDataPoint(double datapoint);
    double getMaximum();

 private:
    unsigned int maximumPersistenceTime;
    double primaryMax, secondaryMax;
    unsigned int primaryAge, secondaryAge;
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class SignalProcessor
{
 public:
    SignalProcessor(double signalLowPass,
                    double derivativeLowPass,
                    unsigned int numAvgSamples,
                    unsigned int numMaxAvgSamples,
                    unsigned int maxPersistenceTime,
                    double maxScale);
    void leadsOn();
    void leadsOff();
    void addDataPoint(double datapoint);
    void setTriggerCallback(void (*callback)());
    std::string getDataString(unsigned long lastIndex,
                              unsigned int signalSelect);

 private:
    std::vector<double> rawSignal,
        filteredSignal,
        signalDerivative,
        filteredDerivative,
        derivativeMaxAverage;
    Averager avg, diffMaxAvg;
    Differentiator diff;
    RollingMaximum diffMax;
    
    double signalLowPassWeight, signalLowPass;
    double derivativeLowPassWeight, derivativeLowPass;

    double maxScale;

    bool leadsAreOff, triggered;

    void (*callback) ();
    std::mutex protection;
};
    

#endif
