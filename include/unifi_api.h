#ifndef UNIFI_API_H
#define UNIFI_API_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

struct NetworkStats {
    float download_mbps;
    float upload_mbps;
    int connected_devices;
    int wired_devices;       // Wired device count
    int wireless_devices;    // Wireless device count
    int stable_device_count; // Smoothed device count to reduce fluctuation
    unsigned long uptime_seconds;
    float ping_ms;
    float packet_loss;
    unsigned long total_bytes_today;
    unsigned long total_bytes_month;
    float monthly_data_gb;  // Monthly data usage in GB (keeping for potential future use)
    String data_source;      // "Total", "RX", "TX" to show what data is displayed
    float uptime_percent;    // WAN availability percentage
    float experience_score;  // Network experience score (0-100)
    String wan_status;
    String wan_ip;
    unsigned long timestamp;
};

class UniFiAPI {
private:
    WiFiClientSecure client;
    String baseUrl;
    String username;
    String password;
    String sessionCookie;
    String csrfToken;
    bool authenticated;
    unsigned long lastAuthTime;
    
    // Helper methods
    String extractCookie(String headers);
    String extractCSRF(String headers);
    bool parseLoginResponse(String response);
    float bytesToMbps(unsigned long bytes, unsigned long timeDelta);
    void getClientStats(NetworkStats& stats);
    void getSystemStats(NetworkStats& stats);
    void getDeviceStats(NetworkStats& stats);
    void getNetworkExperience(NetworkStats& stats);
    
public:
    UniFiAPI();
    ~UniFiAPI();
    
    // Connection management
    bool connect(String url, String user, String pass);
    bool authenticate();
    bool isConnected();
    void disconnect();
    
    // API calls
    NetworkStats getNetworkStats();
    String getSiteHealth();
    int getActiveClients();
    String getDeviceList();
    
    // Utility
    bool testConnection();
    void setInsecure(bool insecure);
};

#endif // UNIFI_API_H