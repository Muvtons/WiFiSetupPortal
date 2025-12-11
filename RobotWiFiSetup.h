#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include "RingBuffer.h"

// Default configuration
#define DEFAULT_AP_SSID "RobotSetupAP"
#define DEFAULT_AP_PASSWORD "securepass123"
#define DEFAULT_AP_CHANNEL 1
#define DEFAULT_DASHBOARD_URL "http://192.168.1.100/dashboard"
#define DEFAULT_CAPTIVE_PORTAL_URL "http://192.168.4.1"
#define MAX_NETWORKS 20
#define SERIAL_BUFFER_SIZE 64
#define COMMAND_BUFFER_SIZE 32

// Function pointer types for callbacks
typedef std::function<void(String)> CommandCallback;
typedef std::function<void(String, bool)> StatusCallback;

class RobotWiFiSetup {
public:
    // Configuration structure
    struct Config {
        const char* ap_ssid = DEFAULT_AP_SSID;
        const char* ap_password = DEFAULT_AP_PASSWORD;
        int ap_channel = DEFAULT_AP_CHANNEL;
        const char* default_dashboard_url = DEFAULT_DASHBOARD_URL;
        const char* captive_portal_url = DEFAULT_CAPTIVE_PORTAL_URL;
        bool enable_captive_portal = true;
        uint16_t server_port = 80;
    };

    // Network info structure
    struct NetworkInfo {
        String ssid;
        int rssi;
        bool secure;
    };

    // Status types
    enum class StatusType {
        BOOTING,
        SCANNING,
        CREDENTIALS_SENT,
        CONNECTED_OK,
        CONNECTED_NO_INTERNET,
        PASSWORD_INCORRECT,
        SSID_NOT_FOUND,
        CONNECTION_FAILED,
        UNKNOWN
    };

private:
    // Hardware resources
    WebServer* server;
    DNSServer dnsServer;
    IPAddress apIP;

    // Configuration
    Config config;
    
    // State variables
    String currentStatus;
    StatusType statusType;
    bool isConnected;
    String robotDashboardURL;
    String targetSSID;
    String targetPassword;
    
    // WiFi scan results
    int numNetworks;
    NetworkInfo networks[MAX_NETWORKS];
    
    // Serial command handling
    RingBuffer<char, SERIAL_BUFFER_SIZE> serialBuffer;
    RingBuffer<String, COMMAND_BUFFER_SIZE> commandBuffer;
    CommandCallback commandCallback;
    StatusCallback statusCallback;
    
    // Single-core synchronization
    portMUX_TYPE coreMutex = portMUX_INITIALIZER_UNLOCKED;
    
    // Web content
    static const char INDEX_HTML[];
    
    // Private methods
    void handleRoot();
    void handleConnect();
    void handleScan();
    void handleStatus();
    void handleNotFound();
    void scanNetworks();
    void processSerialInput();
    void processRPiStatus(const String& msg);
    void notifyStatusChanged();
    const char* getStatusTypeString(StatusType type);

public:
    RobotWiFiSetup();
    ~RobotWiFiSetup();
    
    // Initialization and control
    bool begin(const Config& cfg = Config());
    void handle(); // Must be called in loop()
    
    // Configuration
    void setConfig(const Config& cfg);
    
    // Status and connection info
    String getStatus() const;
    StatusType getStatusType() const;
    bool isConnectedToNetwork() const;
    String getDashboardURL() const;
    
    // Serial command handling
    String getSerialCommand();
    void sendSerialStatus(const String& status);
    void registerCommandCallback(CommandCallback callback);
    void registerStatusCallback(StatusCallback callback);
    
    // Network scanning
    int getAvailableNetworks(NetworkInfo* outNetworks, int maxNetworks);
};
