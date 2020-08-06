#include "SignalProcessor.h"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Averager::Averager(unsigned int numSamples) :
    numSamples(numSamples), total(0)
{
    for (int i=0; i<numSamples; i++) {
        samples.push_back(0);
    }
}

void Averager::addDataPoint(double datapoint)
{
    total += datapoint;
    total -= samples[0];
    samples.pop_front();
    samples.push_back(datapoint);
}

double Averager::getAverage()
{
    return total/numSamples;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Differentiator::Differentiator() : datapoint0(0), datapoint1(0) {}

void Differentiator::addDataPoint(double datapoint)
{
    double d0 = (datapoint1 - datapoint0);
    double d1 = (datapoint  - datapoint1);
    derivative = 0.5*(d0 + d1);
}

double Differentiator::getDerivative()
{
    return derivative;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

RollingMaximum::RollingMaximum(unsigned int maximumPersistenceTime)
    : maximumPersistenceTime(maximumPersistenceTime),
      primaryMax(0), secondaryMax(0),
      primaryAge(0), secondaryAge(0) {}

void RollingMaximum::addDataPoint(double datapoint)
{
    primaryAge++;
    secondaryAge++;

    if (secondaryAge > maximumPersistenceTime)
        secondaryMax = 0;

    if (primaryAge > maximumPersistenceTime) {
        primaryMax = secondaryMax;
        primaryAge = secondaryAge;
    }

    if (datapoint > primaryMax) {
        primaryMax = datapoint;
        primaryAge = 0;
    }
    if (datapoint > secondaryMax) {
        secondaryMax = datapoint;
        secondaryAge = 0;
    }
}

double RollingMaximum::getMaximum()
{
    return primaryMax;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    
