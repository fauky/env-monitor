
#pragma once

#include <DIYables_ESP32_WebServer.h>
#include <WiFi.h>

class Wlan
{
private:
    WiFiAPClass m_wifiAp;

    const char* m_ssid = "EnvGateway";
    const char* m_password = "EnvMonitor";
    IPAddress m_local_ip;
    IPAddress m_gateway;
    IPAddress m_subnet;

    static Wlan* s_instance;

    // Create web server instance
    DIYables_ESP32_WebServer m_server;
    DIYables_ESP32_WebSocket* m_websocket = nullptr;

    // ---------- Circular–buffer configuration ----------
    static const size_t  BUF_COUNT = 50;       // buffer length
    static const size_t  MSG_SIZE  = 256;      // size of each message
    byte                 m_msgBuf[BUF_COUNT][MSG_SIZE];
    size_t               m_writeIndex = 0;     // write marker
    size_t               m_readIndex  = 0;     // read  marker

    // ---------- Circular‑buffer helpers ----------
    void putMessage(const byte* msg, size_t len);
    bool getMessage(byte* outMsg, size_t& outLen);

public:
    Wlan();
    ~Wlan();

    void updateTemps(byte* msg, size_t msg_len);
  
    void tick();

    bool connectToWiFi();
    bool createWiFiAP();
    bool startDhcpServer();
    bool startWebServer();
    bool startWebSocketServer();

    // Webserver event handler
    static void handleHome(WiFiClient& client, const String& method, const String& request, const QueryParams& params, const String& jsonData);
    static void handleChartJs(WiFiClient& client, const String& method, const String& request, const QueryParams& params, const String& jsonData);

    void sendFile(WiFiClient& client, const char* filename, const char* contentType);

    // Websocket event handlers
    static void onWebSocketOpen(net::WebSocket& ws);
    static void onWebSocketMessage(net::WebSocket& ws, const net::WebSocket::DataType dataType, const char* message, uint16_t length);
    static void onWebSocketClose(net::WebSocket& ws, const net::WebSocket::CloseCode code, const char* reason, uint16_t length);

    // Utilities
    static char s_mac[18];
    static const char* getMacAddr();

};
