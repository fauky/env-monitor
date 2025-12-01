
#include "EnvMonitor.h"

EnvMonitor* EnvMonitor::s_instance = nullptr;

EnvMonitor::EnvMonitor(std::function<void()> packetSentCb, std::function<void()> packetReceivedCb)
    : m_onPacketSent(std::move(packetSentCb))
    , m_onPacketReceived(std::move(packetReceivedCb))
    , m_LoRa(EnvMonitor::onPacketSent, EnvMonitor::onPacketReceived)
{
    s_instance = this;
#if defined(MODE_NODE)
    m_device_id = CONFIG_TX_STATION_ID;
    m_temp_update_interval = CONFIG_TX_TEMPERATURE_TIMER;
    m_transmission_interval = CONFIG_TX_TRANSMISSION_TIMER;
#elif defined(MODE_GATEWAY)
    for (int i = 0; i < NUM_STATIONS; i++)
    {
        m_sta_temp[i] = -200.0; // Set it to an invalid temperature
    }
#endif
}

EnvMonitor::~EnvMonitor()
{
}

void EnvMonitor::flashLED()
{
    if ((millis() - m_led_debounce_time) > m_led_debounce_delay) {
        m_led_state = !m_led_state;
        if (m_led_state) {
            digitalWrite(BOARD_LED, LED_ON);
        } else {
            digitalWrite(BOARD_LED, !LED_ON);
        }
        m_led_debounce_time = millis();
    }
}

void EnvMonitor::turnLEDOn()
{
    digitalWrite(BOARD_LED, LED_ON);
}
void EnvMonitor::turnLEDOff()
{
    digitalWrite(BOARD_LED, LED_OFF);
}

bool EnvMonitor::init(unsigned long baud)
{
    Serial.begin(baud);
    Serial.print("\nEnvMonitor::init Station ID: ");
    Serial.println(m_device_id);

#if defined(MODE_NODE)
    m_TempSensor.init();
#endif

    pinMode(RADIO_DIO2_PIN, INPUT);
    pinMode(BOARD_LED, OUTPUT);
    turnLEDOff();

    if(! m_Display.init(I2C_SDA, I2C_SCL, DISPLAY_ADDR))
    {
        Serial.println("EnvMonitor::Failed to initialize display");
        return false;
    }

    Serial.println("EnvMonitor::Display initialized");

    if(! m_LoRa.init())
    {
        Serial.println("EnvMonitor::Failed to initialize LoRa radio");
        return false;
    }
    Serial.println("EnvMonitor::LoRa radio initialized");

#if defined(MODE_GATEWAY)
    m_LoRa.enableReceiver();

    if(! m_Wlan.connectToWiFi())
    {
        Serial.println("EnvMonitor::Failed to start WiFi");
    }
#endif

    m_sd_card_present = m_SDCard.init(SDCARD_SCLK, SDCARD_MISO, SDCARD_MOSI, SDCARD_CS);
    if(m_sd_card_present)
    {
        Serial.println("EnvMonitor::SD card initialized");

#if defined(MODE_NODE)
        m_log_file_seq = m_SDCard.getNextFileSequence("env_monitor_log_");
        snprintf(m_log_file, sizeof(m_log_file), "/env_monitor_log_%04u.csv", m_log_file_seq);
#elif defined(MODE_GATEWAY)
        m_log_file_seq = m_SDCard.getNextFileSequence("env_gateway_log_");
        snprintf(m_log_file, sizeof(m_log_file), "/env_gateway_log_%04u.csv", m_log_file_seq);
#endif
        
        // Write CSV header
        m_SDCard.writeFile(m_log_file, "Device ID, SequenceNumber, Update Interval(s), Temperature (C),...");
    }
    else
    {
        Serial.println("EnvMonitor::Failed to initialize SD card");
    }

    Serial.println("EnvMonitor::Initialization complete");
    return true;
}

bool EnvMonitor::tick()
{
#if defined(MODE_NODE)
    uint32_t now  = millis();
    uint32_t elapsed = now - m_prevUpdateMillis;        // Δ time in ms
    m_prevUpdateMillis = now;                           // remember for next cycle

    /* Count‑down the temperature‑update timer */
    if (m_tempTimer > elapsed) {
        m_tempTimer -= elapsed;                        // still counting down
    } else {
        m_tempTimer = 0;                               // reached zero
    }

    /* Count‑down the temperature‑transmit timer */
    if (m_transmitTimer > elapsed) {
        m_transmitTimer -= elapsed;
    } else {
        m_transmitTimer = 0;
    }

    /* Take action when the timers reach zero */
    if (m_tempTimer == 0) {
        updateTemperature();                           // refresh m_temperature
        m_tempTimer = m_temp_update_interval;          // reset timer
    }

    if (m_transmitTimer == 0) {
        transmitTemperature();
        m_transmitTimer = m_transmission_interval;     // reset timer
    }
#endif
    if(m_packet_received)
    {
        processReceivedPacket();                      // process received packet
        turnLEDOff();
        m_packet_received = false;
    }

#if defined(MODE_GATEWAY)
    // Handle webserver events, provided most recent temperatues
    m_Wlan.tick();
#endif

    return true;
}

void EnvMonitor::onPacketSent()
{
    if (s_instance)
    {
        s_instance->turnLEDOff();

        if (s_instance && s_instance->m_onPacketSent)
        {
            s_instance->m_onPacketSent();
        }
    }
}

void EnvMonitor::onPacketReceived()
{
    if (s_instance)
    {
        s_instance->turnLEDOn();
        s_instance->m_packet_received = true;

        if (s_instance->m_onPacketReceived)
        {
            s_instance->m_onPacketReceived();
        }
    }
}

void EnvMonitor::updateTemperature()
{
#if defined(MODE_NODE)
    // Read temperature from temperature sensor
    m_temperature = m_TempSensor.getTemperature();
    
    // Display current temperature on screen
    updateDisplay();

    // Append temperature to buffer, and transmit if full
    m_temp_buffer[m_temp_buffer_index++] = m_temperature;
    if (m_temp_buffer_index >= TEMP_BUFFER_SIZE)
    {
        transmitTemperature();
        m_temp_buffer_index = 0;
    }
#endif
}

void EnvMonitor::transmitTemperature()
{
#if defined(MODE_NODE)
    /* Create one packet string:
       device_id, sequence_number, temp_update_interval, <temp1>,<temp2>,...,<tempN>
    */
    int index = 0;

    // Append device ID to m_payload array
    memcpy(&m_payload[index], &m_device_id, sizeof(m_device_id));
    index += sizeof(m_device_id);

    // Append sequence number to m_payload array
    memcpy(&m_payload[index], &m_sequence_number, sizeof(m_sequence_number));
    index += sizeof(m_sequence_number);

    // Convert m_temp_update_interval from milliseconds to seconds
    // and append it  to m_payload array
    uint32_t temp_update_interval_seconds = static_cast<uint32_t>(m_temp_update_interval / 1000.0 + 0.5);
    memcpy(&m_payload[index], &temp_update_interval_seconds, sizeof(uint32_t));
    index += sizeof(temp_update_interval_seconds);

    // Append every buffered temperature value.
    for (int i = 0; i < m_temp_buffer_index; ++i)
    {
        memcpy(&m_payload[index], &m_temp_buffer[i], sizeof(m_temp_buffer[i]));
        index += sizeof(m_temp_buffer[i]);
    }

    // Transmit the assembled payload via LoRa
    turnLEDOn();
    m_LoRa.transmit(m_payload, index);

    // Reset buffer index so the next sample pass starts fresh.
    m_temp_buffer_index = 0;

    // Log to SD card if present.
    logMessage(m_payload, index);
#endif
}

void EnvMonitor::processReceivedPacket()
{
    byte packet[256];
    size_t len = sizeof(packet);
    m_LoRa.readData(packet, len);

    if(len)
    {
        m_Wlan.updateTemps(packet, len);
        logMessage(packet, len);
    }
}

void EnvMonitor::parsePayload(byte payload[], size_t len, uint8_t& device_id, uint16_t& sequence_number, uint32_t& temp_update_interval_seconds, float temp_buffer[], size_t& n_temps)
{
    int index = 0;

    // ---------- 1. Device ID (1 byte) ----------
    memcpy(&device_id, &payload[index], sizeof(device_id));
    index += sizeof(device_id);

    // ---------- 2. Sequence number (1 bytes) ----------
    memcpy(&sequence_number, &payload[index], sizeof(sequence_number));
    index += sizeof(sequence_number);

    // ---------- 3. Temperature interval (4 bytes) ----------
    memcpy(&temp_update_interval_seconds, &payload[index], sizeof(temp_update_interval_seconds));
    index += sizeof(temp_update_interval_seconds);

    // ---------- 4. Temperature buffer ----------
    // Determine how many float values remain in the payload
    int remaining = len - index;
    n_temps = remaining / sizeof(float);
    for (int i = 0; i < n_temps; ++i)
    {
        memcpy(&temp_buffer[i], &payload[index], sizeof(float));
        index += sizeof(float);
    }
}

void EnvMonitor::updateDisplay()
{
#if defined(MODE_NODE)
    m_Display.update(CONFIG_TX_STATION_ID, m_temperature, m_transmitTimer, m_transmission_interval);
#elif defined(MODE_GATEWAY)
    m_Display.update(m_sta_temp);
#endif

}

void EnvMonitor::logMessage(byte payload[], size_t len)
{
    uint8_t device_id;
    uint16_t sequence_number;
    uint32_t update_interval;
    float temps[32];
    size_t n_temps;

    parsePayload(payload, len, device_id, sequence_number, update_interval, temps, n_temps);

#if defined(MODE_GATEWAY)    
    // Update Display
    switch (device_id)
    {
        case 1: m_sta_temp[0] = temps[n_temps-1]; break;
        case 2: m_sta_temp[1] = temps[n_temps-1]; break;
        default: break;
    }
    m_Display.update(m_sta_temp);
#endif

// Debug log
    Serial.printf("%d,%u,%lu,", device_id, sequence_number, update_interval);
    for (size_t i = 0; i < n_temps; ++i) {
        Serial.print(temps[i]);
        if (i < n_temps - 1) {
            Serial.print(",");
        }
    }
    Serial.println();

    if(m_sd_card_present)
    {
        m_SDCard.writeFile(m_log_file, device_id, sequence_number, update_interval, temps, n_temps);

        // Rotate log file
        if (m_sequence_number >= MAX_TEMP_SEQUENCE)
        {
            m_sequence_number = 0;

            // Rotate the logfile when enough packets have been sent
            if (m_sd_card_present)
            {
                m_log_file_seq++; // start a new log file

                // Increment the file counter and start a new file
#if defined(MODE_NODE)
                snprintf(m_log_file, sizeof(m_log_file), "/env_monitor_log_%04u.csv", m_log_file_seq);
#elif defined(MODE_GATEWAY)
                snprintf(m_log_file, sizeof(m_log_file), "/env_gateway_log_%04u.csv", m_log_file_seq);
#endif
                m_SDCard.writeFile(m_log_file, "");

                // Write CSV header
                m_SDCard.writeFile(m_log_file, "Device ID, SequenceNumber, Update Interval(s), Temperature (C),...");

            }
        }
        else
        {
            m_sequence_number++;
        }

    }
}

