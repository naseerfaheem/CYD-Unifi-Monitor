#ifndef NETWORK_HANDLER_H
#define NETWORK_HANDLER_H

#include <Arduino.h>
#include <WiFi.h>

class NetworkHandler {
private:
    String ssid;
    String password;
    bool connected;
    unsigned long lastReconnectAttempt;
    int reconnectAttempts;
    
public:
    NetworkHandler();
    ~NetworkHandler();
    
    // Connection management
    bool connect(String ssid, String password);
    void disconnect();
    bool isConnected();
    void maintain();
    
    // Network info
    String getIP();
    int getRSSI();
    String getSSID();
    String getMacAddress();
    
    // Utilities
    void printNetworkInfo();
    bool waitForConnection(unsigned long timeout);
};

#endif // NETWORK_HANDLER_H