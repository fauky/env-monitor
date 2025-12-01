
#if defined(MODE_NODE)
#include "components/sensors/temp-ds18b20.h"

TempSensor::TempSensor(uint8_t pin)
    : m_oneWire(pin)
    , m_sensor(&m_oneWire)
{
}

TempSensor::~TempSensor()
{
}

void TempSensor::init()
{
    m_sensor.begin();

    // wait for sensor readings to stabilize
    delay(500);
}

float TempSensor::getTemperature()
{
    // Request the temperature from the first sensor
    m_sensor.requestTemperaturesByIndex(0);
    return m_sensor.getTempCByIndex(0);
}
#endif