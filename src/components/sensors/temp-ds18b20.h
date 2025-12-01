
#pragma once

#if defined(MODE_NODE)
#include <DallasTemperature.h>

class TempSensor
{
private:
    OneWire m_oneWire;
    DallasTemperature m_sensor;

public:
    TempSensor(uint8_t pin);
    ~TempSensor();

    void init();
    float getTemperature();
};
#endif
