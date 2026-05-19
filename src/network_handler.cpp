#include <Arduino.h>
#include "network_handler.h"
#include "config.h"

NetworkHandler::NetworkHandler() : connected(false), lastReconnectAttempt(0), reconnectAttempts(0) {
}

NetworkHandler::~NetworkHandler() {
    disconnect();
}

bool NetworkHandler::connect(String _ssid, String _password) {
    ssid = _ssid;
    password = _password;
    
    #if DEBUG_SERIAL
    Serial.print("Connecting to WiFi: ");
    Serial.println(ssid);
    #endif
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    
    // Wait for connection
    return waitForConnection(WIFI_CONNECT_TIMEOUT);
}

void NetworkHandler::disconnect() {
    WiFi.disconnect(true);
    connected = false;
}

bool NetworkHandler::isConnected() {
    connected = (WiFi.status() == WL_CONNECTED);
    return connected;
}

void NetworkHandler::maintain() {
    if (!isConnected()) {
        unsigned long now = millis();
        
        // Try to reconnect every 30 seconds
        if (now - lastReconnectAttempt > 30000) {
            lastReconnectAttempt = now;
            reconnectAttempts++;
            
            #if DEBUG_SERIAL
            Serial.printf("WiFi reconnect attempt %d...\n", reconnectAttempts);
            #endif
            
            WiFi.reconnect();
            
            if (waitForConnection(WIFI_CONNECT_TIMEOUT)) {
                reconnectAttempts = 0;
                #if DEBUG_SERIAL
                Serial.println("WiFi reconnected successfully");
                printNetworkInfo();
                #endif
            }
        }
    }
}

String NetworkHandler::getIP() {
    if (isConnected()) {
        return WiFi.localIP().toString();
    }
    return "Not connected";
}

int NetworkHandler::getRSSI() {
    if (isConnected()) {
        return WiFi.RSSI();
    }
    return -100;
}

String NetworkHandler::getSSID() {
    if (isConnected()) {
        return WiFi.SSID();
    }
    return "Not connected";
}

String NetworkHandler::getMacAddress() {
    return WiFi.macAddress();
}

void NetworkHandler::printNetworkInfo() {
    if (isConnected()) {
        Serial.println("\n=== Network Information ===");
        Serial.print("SSID: ");
        Serial.println(WiFi.SSID());
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        Serial.print("Gateway: ");
        Serial.println(WiFi.gatewayIP());
        Serial.print("DNS: ");
        Serial.println(WiFi.dnsIP());
        Serial.print("MAC Address: ");
        Serial.println(WiFi.macAddress());
        Serial.print("RSSI: ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm");
        Serial.println("==========================\n");
    } else {
        Serial.println("Not connected to WiFi");
    }
}

bool NetworkHandler::waitForConnection(unsigned long timeout) {
    unsigned long startTime = millis();
    
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - startTime > timeout) {
            #if DEBUG_SERIAL
            Serial.println("\nWiFi connection timeout");
            #endif
            return false;
        }
        delay(500);
        #if DEBUG_SERIAL
        Serial.print(".");
        #endif
    }
    
    #if DEBUG_SERIAL
    Serial.println("\nWiFi connected!");
    #endif
    
    connected = true;
    return true;
}