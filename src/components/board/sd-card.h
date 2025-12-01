
#pragma once

#include <SD.h>

class SDCard
{
private:
    SPIClass m_SD_SPI {HSPI};

    bool m_SDCard_Valid;
    bool testCard();

public:
    SDCard();
    ~SDCard();

    bool init(int8_t sck, int8_t miso, int8_t mosi, int8_t ss);

    int getNextFileSequence(const char *prefix);
    bool writeFile(const char *path, const char *buffer);
    bool writeFile(const char *path, uint8_t device_id, uint16_t sequence_number
        , uint32_t update_interval, float temps[], size_t n_temps);

    bool readFile(const char *path, uint8_t *buffer, size_t size);
    bool deleteFile(const char *path);
};
