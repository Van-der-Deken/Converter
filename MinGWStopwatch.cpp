#include "MinGWStopwatch.h"

using namespace conv;

void MinGWStopwatch::start()
{
    gettimeofday(&timeValue, NULL);
    previous = (uint64_t)timeValue.tv_sec * 1000 + timeValue.tv_usec / 1000;
}

void MinGWStopwatch::lap()
{
    gettimeofday(&timeValue, NULL);
    uint64_t current = (uint64_t)timeValue.tv_sec * 1000 + timeValue.tv_usec / 1000;
    laps.push_back(current - previous);
    previous = current;
}
