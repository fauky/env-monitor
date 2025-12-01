
#pragma once

#include <U8g2lib.h>

#define U8G2_HOR_ALIGN_CENTER(t)    ((m_pU8G2->getDisplayWidth() -  (m_pU8G2->getUTF8Width(t))) / 2)
#define U8G2_HOR_ALIGN_RIGHT(t)     ( m_pU8G2->getDisplayWidth()  -  m_pU8G2->getUTF8Width(t))

class Display
{
private:
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C* m_pU8G2 = nullptr;

    bool m_display_online;

    // Progress bar dimensions
    const u8g2_uint_t m_bar_height  = 8;
    const u8g2_uint_t m_bar_start_x = 17;
    const u8g2_uint_t m_bar_start_y = 48;

public:
    Display();
    ~Display();

    bool init(int sda, int scl, int address);
    bool IsOnline() const { return m_display_online; }

    bool update(uint8_t id, float temperature, uint32_t tx_timer, uint32_t tx_timer_max);
    bool update(float temperature[]);
};
