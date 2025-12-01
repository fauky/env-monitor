
#include "components/radios/lora.h"

LoRa* LoRa::s_instance = nullptr;

LoRa::LoRa(std::function<void()> callback_send, std::function<void()> callback_receive)
    : m_onPacketSent(callback_send)
    , m_onPacketReceived(callback_receive) // <--- 6. Register this instance
{
    s_instance = this;
    m_radio = new Module(RADIO_CS_PIN, RADIO_DIO0_PIN, RADIO_RST_PIN, RADIO_DIO1_PIN);
}

bool LoRa::init()
{
    SPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN);
    m_radio_state = m_radio.begin();

    if(m_radio_state == RADIOLIB_ERR_NONE)
    {
#if defined(MODE_NODE)
        m_radio.setPacketSentAction(LoRa::onPacketSent);
#elif defined(MODE_GATEWAY)
        m_radio.setPacketReceivedAction(LoRa::onPacketReceived);
#endif
        /* Validate Configurations (valid for SX1276) */
        if ((m_radio_state = m_radio.setFrequency(CONFIG_RADIO_FREQ) != RADIOLIB_ERR_NONE) ||
            (m_radio_state = m_radio.setBandwidth(CONFIG_RADIO_BANDWIDTH)) != RADIOLIB_ERR_NONE ||
            // (m_radio_state = m_radio.setBitRate(CONFIG_RADIO_BITRATE)) != RADIOLIB_ERR_NONE ||
            (m_radio_state = m_radio.setSpreadingFactor(CONFIG_RADIO_SPREAD_FACTOR)) != RADIOLIB_ERR_NONE ||
            (m_radio_state = m_radio.setCodingRate(CONFIG_RADIO_CODING_RATE)) != RADIOLIB_ERR_NONE ||
            (m_radio_state = m_radio.setSyncWord(CONFIG_RADIO_SYNC_WORD)) != RADIOLIB_ERR_NONE ||
            (m_radio_state = m_radio.setOutputPower(CONFIG_RADIO_OUTPUT_POWER)) != RADIOLIB_ERR_NONE ||
            (m_radio_state = m_radio.setCurrentLimit(CONFIG_RADIO_CURRENT_LIMIT)) != RADIOLIB_ERR_NONE ||
            (m_radio_state = m_radio.setPreambleLength(CONFIG_RADIO_PREAMBLE)) != RADIOLIB_ERR_NONE ||
            (m_radio_state = m_radio.setCRC(false)) != RADIOLIB_ERR_NONE)
            
        {
            Serial.print("LoRa::init(): Failed to initialize LoRa module. Error code: ");
            Serial.println(m_radio_state);
            return false;
        }
    }
    
    return (m_radio_state == RADIOLIB_ERR_NONE);
}

LoRa::~LoRa()
{
}

bool LoRa::transmit(byte data[], size_t length)
{
    m_radio_state = m_radio.startTransmit(data, length);

    return (m_radio_state == RADIOLIB_ERR_NONE);
}

bool LoRa::readData(byte data[], size_t& length)
{
    length = m_radio.getPacketLength();
    return (m_radio.readData(data, length) == RADIOLIB_ERR_NONE);
}

bool LoRa::enableReceiver()
{
    return (m_radio.startReceive() == RADIOLIB_ERR_NONE);
}

void LoRa::onPacketSent()
{
    if (s_instance && s_instance->m_onPacketSent)
        s_instance->m_onPacketSent();
}

void LoRa::onPacketReceived()
{
    if (s_instance && s_instance->m_onPacketReceived)
        s_instance->m_onPacketReceived();
}

