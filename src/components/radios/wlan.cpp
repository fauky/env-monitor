
#include "components/radios/wlan.h"
#include "config/websocket_html.h"
#include "config/board_config.h"
#include "esp_netif.h"
#include "lwip/dhcp.h"
#include <cstring>
#include <SPIFFS.h>

Wlan* Wlan::s_instance = nullptr;
char Wlan::s_mac[18];

Wlan::Wlan()
    : m_local_ip(192, 168, 1, 1)
    , m_gateway(192, 168, 1, 1)
    , m_subnet(255, 255, 255, 0)
{
    s_instance = this;
}

Wlan::~Wlan()
{
}

void Wlan::updateTemps(byte* msg, size_t msg_len)
{
    // Store the incoming message in the circular buffer
    putMessage(msg, msg_len);
}

void Wlan::tick()
{
    m_server.handleWebSocket();   // Handle WebSocket first (high priority for real-time communication)
    m_server.handleClient();      // Then handle HTTP requests
}

bool Wlan::connectToWiFi()
{
    Serial.print("connectToWiFi::Connecting to WiFi");
    WiFi.begin(CONFIG_WIFI_SSID, CONFIG_WIFI_PASSWORD);
    
    int max_retries = CONFIG_WIFI_MAX_RETRIES;
    while(max_retries && WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.print(".");
        max_retries--;
    }
    Serial.println();

    if(WiFi.status() != WL_CONNECTED)
    {
        Serial.println("connectToWiFi::Failed to connect to WiFi");
        return false;
    }

    Serial.println("connectToWiFi::Connected to WiFi");
    return startWebServer();
}

bool Wlan::createWiFiAP()
{
    // Initialize SPIFFS
    if (!SPIFFS.begin(true))
    { // true means format the filesystem if it fails to mount
        Serial.println("Wlan::SPIFFS Mount Failed");
    }
    else
    {
        Serial.println("Wlan::SPIFFS Mounted successfully");
    }

    // Enable AP mode
    WiFi.mode(WIFI_AP);

    // Assign the static IP configuration (AP IP, gateway & subnet)
    WiFi.softAPConfig(m_local_ip, m_gateway, m_subnet);

    // Start the soft‑AP with the defined SSID and password
    if (WiFi.softAP(m_ssid, m_password))
    {
        // Display AP Info
        Serial.println("Wlan::WiFi Access Point started.");
        Serial.print("Wlan::SSID: ");
        Serial.println(m_ssid);
        Serial.print("Wlan::Password: ");
        Serial.println(m_password);
        Serial.print("Wlan::IP Address: ");
        Serial.println(m_local_ip);
        Serial.print("Wlan::Gateway: ");
        Serial.println(m_gateway);
        Serial.print("Wlan::Netmask: ");
        Serial.println(m_subnet);

        startDhcpServer();
        startWebServer();

        return true;
    }

    return false;
}

bool Wlan::startDhcpServer()
{
    // Get the netif handle for the soft‑AP interface
    esp_netif_t *ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    if (ap_netif && esp_netif_dhcps_start(ap_netif) == ESP_OK) {
        Serial.println("Wlan::DHCP server started");
        return true;
    }
    else
    {
        Serial.println("Wlan::DHCP server failed to start");
        return false;
    }
}

bool Wlan::startWebServer()
{
    // Mount SPIFFS file storage
    if (!SPIFFS.begin(true))
    {
        Serial.println("Wlan::SPIFFS Mount Failed");
    }
    else
    {
        Serial.println("Wlan::SPIFFS Mounted successfully");
    }

    // Configure web server routes
    m_server.addRoute("/", Wlan::handleHome);
    m_server.addRoute("/chart.js", Wlan::handleChartJs);
    m_server.begin();

    return startWebSocketServer();
}

bool Wlan::startWebSocketServer()
{
    // Enable WebSocket functionality
    m_websocket = m_server.enableWebSocket(81);
    if (m_websocket == nullptr)
    {
        Serial.println("Failed to start WebSocket server");
        return false;
    }

    // Set up WebSocket event handlers
    m_websocket->onOpen(Wlan::onWebSocketOpen);
    m_websocket->onMessage(Wlan::onWebSocketMessage);
    m_websocket->onClose(Wlan::onWebSocketClose);
    Serial.println("WebSocket server started on port 81");

    return true;
}


// Page handlers
void Wlan::handleHome(WiFiClient& client, const String& method, const String& request, const QueryParams& params, const String& jsonData)
{
    if(s_instance)
    {
        s_instance->m_server.sendResponse(client, htmlPage);
    }
}

void Wlan::handleChartJs(WiFiClient& client, const String& method, const String& request, const QueryParams& params, const String& jsonData)
{
    if(s_instance)
    {
        s_instance->sendFile(client, "/chart.js", "application/javascript");
    }
}

void Wlan::sendFile(WiFiClient& client, const char* filename, const char* contentType)
{
    File file = SPIFFS.open(filename, "r");
    
    if (file) {
        Serial.printf("Wlan::sendFile: %s", filename);

        client.println("HTTP/1.1 200 OK");
        client.print("Content-Type: ");
        client.println(contentType);
        client.println("Connection: close");
        client.println();

        // Stream file in 1024 byte chunks.
        const size_t bufferSize = 1024;
        char buffer[bufferSize];

        while (file.available())
        {
            size_t bytesRead = file.readBytes(buffer, bufferSize);
            client.write(buffer, bytesRead);
        }
        file.close();
    }
    else
    {
        s_instance->m_server.send404(client);
        Serial.printf("Failed to open %s file for reading\n", filename);
    }
}

// WebSocket event handlers
void Wlan::onWebSocketOpen(net::WebSocket& ws) {
    Serial.println("onWebSocketOpen::New WebSocket connection");
}

void Wlan::onWebSocketMessage(net::WebSocket& ws, const net::WebSocket::DataType dataType, const char* message, uint16_t length) {
    String msgStr = String(message);
    String response = "";

    // Command processing
    if (s_instance && msgStr.equalsIgnoreCase("refresh"))
    {
        // Retrieve the oldest stored message (if any) and broadcast it
        byte msgOut[MSG_SIZE];
        size_t msgLen = 0;
        if (s_instance->getMessage(msgOut, msgLen))
        {
            if (s_instance->m_websocket)
            {
                s_instance->m_websocket->broadcastBIN(msgOut, msgLen);
            }
        }
    }
}

void Wlan::onWebSocketClose(net::WebSocket& ws, const net::WebSocket::CloseCode code, const char* reason, uint16_t length)
{
    Serial.println("WebSocket client disconnected");
}

const char* Wlan::getMacAddr()
{
    return s_mac;
}

void Wlan::putMessage(const byte* msg, size_t len)
{
    if (!msg) return;

    // Clamp length so it fits in a single byte
    uint8_t storeLen = (len > 255) ? 255 : static_cast<uint8_t>(len);

    // Store the length byte first
    m_msgBuf[m_writeIndex][0] = storeLen;

    // Copy the payload following the length byte
    memcpy(m_msgBuf[m_writeIndex] + 1, msg, storeLen);

    // Advance the write index
    size_t nextWrite = (m_writeIndex + 1) % BUF_COUNT;

    // If writing would overwrite the unread element, bump the read index
    if (nextWrite == m_readIndex) {
        Serial.println(F("Wlan::buffer overflow"));
        m_readIndex = (m_readIndex + 1) % BUF_COUNT;
    }
    m_writeIndex = nextWrite;
}

bool Wlan::getMessage(byte* outMsg, size_t& outLen)
{
    if (!outMsg) return false;

    // Empty buffer?
    if (m_writeIndex == m_readIndex) {
        Serial.println(F("Wlan::buffer empty"));
        outLen = 0;
        return false;
    }

    // Retrieve the stored length
    uint8_t storedLen = m_msgBuf[m_readIndex][0];
    // Guard against invalid length (should never happen but is safe)
    if (storedLen == 0 || storedLen > 255) {
        Serial.println(F("GET: invalid length - resetting buffer"));
        // Reset indices to avoid infinite loop
        m_writeIndex = m_readIndex;
        outLen = 0;
        return false;
    }

    // Copy the payload leaving out the length byte
    memcpy(outMsg, m_msgBuf[m_readIndex] + 1, storedLen);
    outLen = storedLen;

    // Advance read index
    m_readIndex = (m_readIndex + 1) % BUF_COUNT;

    return true;
}
