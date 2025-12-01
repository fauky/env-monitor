
#pragma once

#include "config/board_config.h"
// #include <SPI.h>
#include <RadioLib.h>
#include <functional>

class LoRa
{
private:
    SX1276 m_radio = nullptr;
    int m_radio_state = RADIOLIB_ERR_UNKNOWN;

    static LoRa* s_instance;
    std::function<void()> m_onPacketSent;
    std::function<void()> m_onPacketReceived;

    String default_payload = "12,34,56,78,90,123456789012345";

public:
    LoRa(std::function<void()> callback_send = nullptr, std::function<void()> callback_receive = nullptr);

    ~LoRa();

    bool init();

    bool transmit(byte data[], size_t length);
    bool readData(byte data[], size_t& length);

    bool enableReceiver();
    static void onPacketSent();
    static void onPacketReceived();
};
