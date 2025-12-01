
#pragma once

#include "config/board_config.h"
#include "components/board/sd-card.h"
#include "components/board/display.h"
#include "components/radios/lora.h"
#include "components/radios/wlan.h"

#if defined(MODE_NODE)
#include "components/sensors/temp-ds18b20.h"
#elif defined(MODE_GATEWAY)
#endif


class EnvMonitor
{
private:
    static EnvMonitor* s_instance;

    SDCard m_SDCard;
    Display m_Display;
    LoRa m_LoRa;
    Wlan m_Wlan;

#if defined(MODE_NODE)
    TempSensor m_TempSensor{ TEMP_SENSOR_PIN };
#elif defined(MODE_GATEWAY)
    static const uint8_t NUM_STATIONS = 2;
    float m_sta_temp[NUM_STATIONS];  // Received Temperature in Celsius
#endif

    uint8_t m_device_id;    // Unique device ID

    static const uint32_t m_led_debounce_delay = 50;
    uint32_t m_led_debounce_time = 0;
    uint8_t m_led_state = LOW;

    std::function<void()> m_onPacketSent = nullptr;
    std::function<void()> m_onPacketReceived = nullptr;

    // Countdown timers (ms remaining before next action)
    uint32_t m_prevUpdateMillis;      // last time update() was called
    uint32_t m_tempTimer;             // remaining time before temp update
    uint32_t m_transmitTimer;         // remaining time before temp transmit

    uint32_t m_temp_update_interval;  // temperature update interval in milliseconds
    uint32_t m_transmission_interval; // transmission interval in milliseconds

    bool m_packet_received = false;   // flag is set when packet is received
    bool m_sd_card_present = false;   // is SD card present?

    uint8_t m_temp_buffer_index = 0;                // index of next temperature to write
    float m_temperature;                            // current temperature
    float m_temp_buffer[TEMP_BUFFER_SIZE];           // temperature buffer
    byte m_payload[MAX_PACKET_SIZE];                // LoRa packet payload

    char m_log_file[32];                            // file name is generated at runtime
    uint8_t m_log_file_seq = 1;                     // sequence number for log file

    uint16_t m_sequence_number = 1;                 // sequence number of temperature reading

public:
    EnvMonitor(std::function<void()> packetSentCb, std::function<void()> packetReceivedCb);

    ~EnvMonitor();

    bool init(unsigned long baud);

    void flashLED();
    void turnLEDOn();
    void turnLEDOff();

    void logMessage(byte payload[], size_t len);
    void parsePayload(byte payload[], size_t len
        , uint8_t& device_id, uint16_t& sequence_number, uint32_t& temp_update_interval_seconds
        , float temp_buffer[], size_t& n_temps);

    void updateDisplay();

    static void onPacketSent();
    static void onPacketReceived();

    void processReceivedPacket();
    bool readData(String& payload);
    
    bool tick();
    void updateTemperature();
    void transmitTemperature();
    bool transmit(String& data);
    bool transmit(const char* data, size_t length);
    bool transmit(const uint8_t data[], size_t length);
};
