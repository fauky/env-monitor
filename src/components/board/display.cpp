
#include "components/board/display.h"
#include <Wire.h>

Display::Display()
{
}
Display::~Display()
{
    delete m_pU8G2;
}

bool Display::init(int sda, int scl, int address)
{
    if(m_pU8G2)
    {
        delete m_pU8G2;
        m_pU8G2 = nullptr;
    }

    Wire.begin(sda, scl);
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
        m_pU8G2 = new U8G2_SSD1306_128X64_NONAME_F_HW_I2C(U8G2_R0, U8X8_PIN_NONE);
        m_display_online = true;
        m_pU8G2->begin();

        m_pU8G2->clearBuffer();
        m_pU8G2->drawRFrame(0, 0, 128, 64, 5);
        m_pU8G2->setFont(u8g2_font_pxplusibmvga8_mr);

        // --- Title ----------------------------------------------------
        m_pU8G2->setCursor(17, 15);
#if defined(MODE_NODE)
        m_pU8G2->print("Env. Monitor");
#elif defined(MODE_GATEWAY)
        m_pU8G2->print("Env. Gateway");
#endif
        m_pU8G2->drawHLine(17, 20, m_pU8G2->getDisplayWidth() - 34);

#if defined(MODE_NODE)
        // --- Temperature -----------------------------------------------
        m_pU8G2->setCursor(22, 37);
        m_pU8G2->print("T0: --- C");

        /* ---------- Progress bar for tx_timer ------------------------ */
        const uint16_t bar_width_max = m_pU8G2->getDisplayWidth() - 2 * m_bar_start_x;

        /* Draw the outer frame (unfilled bar) */
        m_pU8G2->drawFrame(m_bar_start_x, m_bar_start_y, bar_width_max, m_bar_height);

        /* Draw the filled portion */
        m_pU8G2->drawBox(m_bar_start_x + 2, m_bar_start_y + 2, bar_width_max - 4, m_bar_height - 4);
#elif defined(MODE_GATEWAY)

        // --- Temperature -----------------------------------------------
        m_pU8G2->setCursor(22, 37);
        m_pU8G2->print("T1: --- C");
        m_pU8G2->setCursor(22, 54);
        m_pU8G2->print("T2: --- C");
#endif
        m_pU8G2->sendBuffer();

    }
    else
    {
        m_display_online = false;
    }

    return m_display_online;
}

// Update the display with new temperature data
bool Display::update(uint8_t id, float temperature, uint32_t tx_timer, uint32_t tx_timer_max)
{
    if(!m_display_online)
        return false;

    /* ---------- Progress bar for tx_timer ------------------------ */
    const uint16_t bar_width_max = m_pU8G2->getDisplayWidth() - 2 * m_bar_start_x;       // width of the bar (px)

    /* Compute filled part width */
    uint16_t filled_width = (tx_timer >= tx_timer_max)
        ? 0
        : bar_width_max - static_cast<uint16_t>( (tx_timer * bar_width_max) / tx_timer_max );

    // Update display
    m_pU8G2->clearBuffer();
    m_pU8G2->drawRFrame(0, 0, 128, 64, 5);
    m_pU8G2->setFont(u8g2_font_pxplusibmvga8_mr);

    // --- Title ----------------------------------------------------
    m_pU8G2->setCursor(17, 15);
    m_pU8G2->print("Env. Monitor");
    m_pU8G2->drawHLine(17, 20, m_pU8G2->getDisplayWidth() - 34);

    // --- Temperature -----------------------------------------------
    char tempBuf[16];
    if (temperature > -100.0)
        snprintf(tempBuf, sizeof(tempBuf), "T%1u: %2.1f C", id, temperature);
    else
        snprintf(tempBuf, sizeof(tempBuf), "T%1u: --- C", id);
    m_pU8G2->setCursor(22, 37);
    m_pU8G2->print(tempBuf);

    /* Draw the outer frame (unfilled bar) */
    m_pU8G2->drawFrame(m_bar_start_x, m_bar_start_y, bar_width_max, m_bar_height);

    /* Draw the filled portion */
    if (filled_width > 0)
    {
        m_pU8G2->drawBox(m_bar_start_x + 2, m_bar_start_y + 2, filled_width - 4, m_bar_height - 4);
    }

    m_pU8G2->sendBuffer();

    return true;
}

// Update the display with new received data
bool Display::update(float temperature[])
{
    if(!m_display_online)
        return false;

    // Update display
    m_pU8G2->clearBuffer();
    m_pU8G2->drawRFrame(0, 0, 128, 64, 5);
    m_pU8G2->setFont(u8g2_font_pxplusibmvga8_mr);

    // --- Title ----------------------------------------------------
    m_pU8G2->setCursor(17, 15);
    m_pU8G2->print("Env. Gateway");
    m_pU8G2->drawHLine(17, 20, m_pU8G2->getDisplayWidth() - 34);

    // --- Temperature -----------------------------------------------
    char tempBuf[16];
    // Station 1
    if (temperature[0] > -100.0)
        snprintf(tempBuf, sizeof(tempBuf), "T1: %2.1f C", temperature[0]);
    else
        snprintf(tempBuf, sizeof(tempBuf), "T1: --- C");
    m_pU8G2->setCursor(22, 37);
    m_pU8G2->print(tempBuf);

    // Station 2
    if (temperature[1] > -100.0)
        snprintf(tempBuf, sizeof(tempBuf), "T2: %2.1f C", temperature[1]);
    else
        snprintf(tempBuf, sizeof(tempBuf), "T2: --- C");
    m_pU8G2->setCursor(22, 54);
    m_pU8G2->print(tempBuf);
    
    m_pU8G2->sendBuffer();

    return true;
}
