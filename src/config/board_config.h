#pragma once

// -- WiFi Connection Info ----------------------------------------------------
#define CONFIG_WIFI_SSID           "EnvGateway"
#define CONFIG_WIFI_PASSWORD       "EnvMonitor"
#define CONFIG_WIFI_MAX_RETRIES     5

// --  LoRa Transmission Settings
#define CONFIG_RADIO_FREQ           915
#define CONFIG_RADIO_BANDWIDTH      500
#define CONFIG_RADIO_BITRATE        300
#define CONFIG_RADIO_SPREAD_FACTOR  10
#define CONFIG_RADIO_PREAMBLE       16
#define CONFIG_RADIO_CODING_RATE    6
#define CONFIG_RADIO_SYNC_WORD      0xAB
#define CONFIG_RADIO_OUTPUT_POWER   17
#define CONFIG_RADIO_CURRENT_LIMIT  140

// --  Display settings ---------------------------------------------------------
#define DISPLAY_MODEL_SSD_LIB   SSD1306Wire
#define DISPLAY_MODEL           U8G2_SSD1306_128X64_NONAME_F_HW_I2C
#define DISPLAY_ADDR          0x3C

// --  I2C configuration -------------------------------------------------------
#define I2C_SDA                 21
#define I2C_SCL                 22

// --  Radio pins --------------------------------------------------------------
#define RADIO_SCLK_PIN          5
#define RADIO_MISO_PIN          19
#define RADIO_MOSI_PIN          27
#define RADIO_CS_PIN            18
#define RADIO_DIO0_PIN          26
#define RADIO_DIO1_PIN          33
#define RADIO_DIO2_PIN          32
#define RADIO_RST_PIN           23

// --  SD Card Pins-------------------------------------------------------------
#define SDCARD_MISO              2
#define SDCARD_MOSI             15
#define SDCARD_SCLK             14
#define SDCARD_CS               13

// --  Misc. pin definitions ---------------------------------------------------
#define BOARD_LED               25
#define LED_ON                 HIGH
#define LED_OFF                LOW

// --  Temperature sensor pin --------------------------------------------------
#define TEMP_SENSOR_PIN         4

// --  Transmission Timers
#define CONFIG_TX_STATION_ID          2       // Unique station identifier
#define CONFIG_TX_TEMPERATURE_TIMER   1000    // Temperature update interval (ms)
#define CONFIG_TX_TRANSMISSION_TIMER  5000    // LoRa transmission interval (ms)

// -- RS232 Debug console -----------------------------------------------------
#define CONFIG_RS232_BAUD         9600      // RS232 Baud rate

// -- Other constants
#define TEMP_BUFFER_SIZE          32        // buffer size for temperature data
#define MAX_PACKET_SIZE           256       // LoRa packet length
#define MAX_TEMP_SEQUENCE         10000     // rotate logfile at this packet count

