
#include "components/board/sd-card.h"

SDCard::SDCard()
{

}

bool SDCard::init(int8_t sck, int8_t miso, int8_t mosi, int8_t ss)
{
    m_SD_SPI.begin(sck, miso, mosi);

    return (SD.begin(ss, m_SD_SPI) && testCard());
}

SDCard::~SDCard()
{
}


bool SDCard::testCard()
{
    const char *path = "/test.txt";
    const char *message = "Environmental Monitor SD card test file";
    uint8_t buffer[128] = {0};

    if (!writeFile(path, message))
    {
        return false;
    }
    delay(100);

    readFile(path, buffer, 128);

    if (memcmp(buffer, message, strlen(message)) != 0)
    {
        return false;
    }

    return deleteFile(path);
}

int SDCard::getNextFileSequence(const char *prefix)
{
    // Define the maximum sequence number
    int next_sequence_number = 0;
    int len = strlen(prefix);

    // Open the root directory of the SD card
    File root = SD.open("/");

    if (root)
    {
        while (true)
        {
            File entry = root.openNextFile();
            if (!entry)
            {
                // No more files
                break;
            }

            // Get the file name
            const char* name = entry.name();
            
            // Check if the file starts with "env_monitor_log_"
            if (strncmp(name, prefix, len) == 0)
            {
                // Extract the sequence number from the filename
                char seqStr[5]; // To hold the 4-digit sequence number + null terminator
                strncpy(seqStr, name + len, 4);
                seqStr[4] = '\0';  // Null-terminate the string
                
                // Convert to an integer and check if it's a new max sequence number
                int seq = atoi(seqStr);
                if (seq > next_sequence_number)
                {
                    next_sequence_number = seq;
                }
            }
            entry.close();  // Close the current file
        }
    } else {
        Serial.println("Failed to open root directory.");
    }

    return next_sequence_number + 1;
}

bool SDCard::writeFile(const char *path
    , uint8_t device_id, uint16_t sequence_number, uint32_t update_interval
    , float temps[], size_t n_temps)
{
    File file;

    if (SD.exists(path)) {
        file = SD.open(path, FILE_APPEND); // Open in append mode if it exists
    } else {
        file = SD.open(path, FILE_WRITE);  // Create a new file if it does not exist
    }

    if (file)
    {
        file.print(device_id);
        file.print(",");
        file.print(sequence_number);
        file.print(",");
        file.print(update_interval);
        for(size_t i=0; i<n_temps; i++)
        {
            file.print(",");
            file.print(temps[i]);
        }
        file.println();
        file.close();
        return true;
    }

    return false;
   
}

bool SDCard::writeFile(const char *path, const char *buffer)
{
    File file;

    if (SD.exists(path)) {
        file = SD.open(path, FILE_APPEND); // Open in append mode if it exists
    } else {
        file = SD.open(path, FILE_WRITE);  // Create a new file if it does not exist
    }

    if (file)
    {
        file.println(buffer);
        file.close();
        return true;
    }

    return false;
}

bool SDCard::readFile(const char *path, uint8_t *buffer, size_t size)
{
    File file = SD.open(path, FILE_READ);
    if (file)
    {
        file.read(buffer, size);
        file.close();
        return true;
    }
    return false;
}

bool SDCard::deleteFile(const char *path)
{
    return (SD.exists(path) && SD.remove(path));
}
