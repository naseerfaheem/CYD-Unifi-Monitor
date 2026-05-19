#include <Arduino.h>
#include "unifi_api.h"
#include "config.h"
#include "credentials.h"
#include <HTTPClient.h>

UniFiAPI::UniFiAPI() : authenticated(false), lastAuthTime(0) {
    #if UNIFI_SKIP_SSL_VERIFY
    client.setInsecure();
    #endif
}

UniFiAPI::~UniFiAPI() {
    disconnect();
}

bool UniFiAPI::connect(String url, String user, String pass) {
    baseUrl = url;
    username = user;
    password = pass;
    
    // Remove trailing slash if present
    if (baseUrl.endsWith("/")) {
        baseUrl = baseUrl.substring(0, baseUrl.length() - 1);
    }
    
    return authenticate();
}

bool UniFiAPI::authenticate() {
    HTTPClient https;
    
    String loginUrl = baseUrl + UNIFI_API_LOGIN;
    
    #if DEBUG_API
    Serial.println("Authenticating with UniFi...");
    Serial.println("URL: " + loginUrl);
    #endif
    
    if (!https.begin(client, loginUrl)) {
        Serial.println("Failed to begin HTTPS connection");
        return false;
    }
    
    https.addHeader("Content-Type", "application/json");
    const char* headerKeys[] = {"Set-Cookie", "X-Csrf-Token"};
    https.collectHeaders(headerKeys, 2);
    
    // Create login JSON
    JsonDocument doc;
    doc["username"] = username;
    doc["password"] = password;
    doc["remember"] = false;
    
    String requestBody;
    serializeJson(doc, requestBody);
    
    #if DEBUG_API
    Serial.println("Login request: " + requestBody);
    #endif
    
    int httpCode = https.POST(requestBody);
    
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String response = https.getString();
        
        // Extract cookies and CSRF token
        if (https.hasHeader("Set-Cookie")) {
            String cookies = https.header("Set-Cookie");
            
            // Extract TOKEN cookie for UDM
            int tokenStart = cookies.indexOf("TOKEN=");
            if (tokenStart != -1) {
                int tokenEnd = cookies.indexOf(";", tokenStart);
                if (tokenEnd == -1) tokenEnd = cookies.length();
                sessionCookie = cookies.substring(tokenStart, tokenEnd);
                
                #if DEBUG_API
                Serial.println("Got TOKEN cookie (first 60 chars): " + sessionCookie.substring(0, 60));
                #endif
            }
        }
        
        // Extract CSRF token
        if (https.hasHeader("X-Csrf-Token")) {
            csrfToken = https.header("X-Csrf-Token");
            #if DEBUG_API
            Serial.println("Got CSRF token: " + csrfToken);
            #endif
        }
        
        // Parse response for CSRF token
        JsonDocument responseDoc;
        DeserializationError error = deserializeJson(responseDoc, response);
        
        if (!error) {
            // Some UniFi versions return CSRF token in response
            if (responseDoc["meta"]) {
                csrfToken = responseDoc["meta"]["csrf_token"] | "";
            }
            
            authenticated = true;
            lastAuthTime = millis();
            
            #if DEBUG_API
            Serial.println("Authentication successful");
            #endif
            
            https.end();
            return true;
        }
    }
    
    #if DEBUG_API
    Serial.printf("Authentication failed. HTTP code: %d\n", httpCode);
    Serial.println("Response: " + https.getString());
    #endif
    
    https.end();
    return false;
}

bool UniFiAPI::isConnected() {
    // Re-authenticate if session is older than 20 minutes
    if (authenticated && (millis() - lastAuthTime > 1200000)) {
        authenticated = authenticate();
    }
    return authenticated;
}

void UniFiAPI::disconnect() {
    authenticated = false;
    sessionCookie = "";
    csrfToken = "";
}

// Main function to get all network stats
NetworkStats UniFiAPI::getNetworkStats() {
    NetworkStats stats = {0};
    stats.timestamp = millis();
    stats.monthly_data_gb = 0;  // Initialize monthly data
    stats.data_source = "Data/Mo";  // Initialize data source label with default
    stats.connected_devices = 0;    // Explicitly initialize device count
    stats.uptime_percent = 0;       // Initialize uptime
    stats.experience_score = 0;     // Initialize experience score
    
    if (!isConnected()) {
        Serial.println("Not connected to UniFi");
        return stats;
    }
    
    // ULTRA FAST - only bandwidth data for maximum touch responsiveness
    Serial.println("\n=== Getting Network Stats (Bandwidth Only) ===");

    // ONLY get device stats - has bandwidth + ping
    getDeviceStats(stats); // Bandwidth, ping from WAN device only
    Serial.println("After getDeviceStats: Monthly GB = " + String(stats.monthly_data_gb) + ", Source = " + stats.data_source);

    // DISABLE device counting for touch responsiveness - will show monthly data instead
    stats.connected_devices = 0; // Disable device count display
    Serial.println("Device counting disabled - showing monthly data usage instead");

    // Set reasonable defaults for other metrics
    if (stats.experience_score == 0 || stats.experience_score > 99) {
        // Calculate realistic experience from available metrics
        float experience = 75.0; // Base score
        if (stats.ping_ms > 0 && stats.ping_ms < 50) experience += 15;
        else if (stats.ping_ms < 100) experience += 5;
        if (stats.uptime_percent > 99) experience += 10;
        stats.experience_score = experience;
        Serial.println("Calculated experience: " + String(stats.experience_score) + "%");
    }

    Serial.println("Final data: Devices DISABLED, " + String(stats.uptime_percent) + "% uptime, " + String(stats.experience_score) + "% experience");
    
    // Calculate monthly data usage (convert from bytes to GB)
    if (stats.total_bytes_month > 0) {
        stats.monthly_data_gb = stats.total_bytes_month / (1024.0 * 1024.0 * 1024.0);
    } else {
        // Estimate based on current rate if no monthly data available
        // Assuming 30 days average usage based on current throughput
        float avgMbps = (stats.download_mbps + stats.upload_mbps) / 2.0;
        if (avgMbps > 0) {
            // Convert Mbps to GB per month (30 days)
            stats.monthly_data_gb = (avgMbps * 30 * 24 * 3600) / (8.0 * 1024.0);
        }
    }
    
    // Always show final stats
    Serial.printf("Final Stats: Down: %.2f Mbps, Up: %.2f Mbps, Devices: %d, Ping: %.1f\n",
                 stats.download_mbps, stats.upload_mbps, stats.connected_devices, stats.ping_ms);
    Serial.printf("  Monthly: %.2f GB, Source: %s, Uptime: %.1f%%\n",
                 stats.monthly_data_gb, stats.data_source.c_str(), stats.uptime_percent);
    
    return stats;
}

// Get client statistics
void UniFiAPI::getClientStats(NetworkStats& stats) {
    HTTPClient https;
    // Use stat/sta endpoint for accurate connected device count
    String clientsUrl = baseUrl + "/proxy/network/api/s/default/stat/sta";
    
    #if DEBUG_API
    Serial.println("Fetching client stats from: " + clientsUrl);
    #endif
    
    if (!https.begin(client, clientsUrl)) {
        Serial.println("Failed to begin HTTPS for clients");
        return;
    }
    
    // Always add authentication headers
    if (!sessionCookie.isEmpty()) {
        https.addHeader("Cookie", sessionCookie);
    }
    if (!csrfToken.isEmpty()) {
        https.addHeader("X-Csrf-Token", csrfToken);
    }
    #if DEBUG_API
    if (!sessionCookie.isEmpty()) {
        Serial.println("Using auth cookie: " + sessionCookie.substring(0, 40) + "...");
    }
    #endif
    
    // ULTRA FAST timeout for touch responsiveness
    https.setTimeout(1000);   // 1 second timeout - maximum responsiveness
    
    int httpCode = https.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        // Get response size
        int len = https.getSize();
        Serial.println("Response size: " + String(len) + " bytes");
        
        // Get stream for reading chunks
        WiFiClient* stream = https.getStreamPtr();
        
        int deviceCount = 0;
        int wiredCount = 0; 
        int wirelessCount = 0;
        
        // Process response in chunks to avoid stack overflow
        const int chunkSize = 1024;  // Larger chunk for better performance
        uint8_t* chunk = new uint8_t[chunkSize + 1];
        String searchBuffer = "";
        int totalBytesRead = 0;
        
        // Read entire response
        while (stream->available()) {
            int bytesRead = stream->readBytes(chunk, chunkSize);
            totalBytesRead += bytesRead;
            chunk[bytesRead] = '\0';
            
            // Add to search buffer
            searchBuffer += String((char*)chunk);
            
            // Process every 4KB to prevent memory issues
            if (searchBuffer.length() > 4096) {
                // Count in this chunk but don't clear all - keep overlap
                String processChunk = searchBuffer.substring(0, searchBuffer.length() - 200);
                
                int idx = 0;
                while ((idx = processChunk.indexOf("\"mac\":\"", idx)) != -1) {
                    deviceCount++;
                    idx += 7;
                }
                
                idx = 0;
                while ((idx = processChunk.indexOf("\"is_wired\":true", idx)) != -1) {
                    wiredCount++;
                    idx += 15;
                }
                
                idx = 0;
                while ((idx = processChunk.indexOf("\"is_wired\":false", idx)) != -1) {
                    wirelessCount++;
                    idx += 16;
                }
                
                // Keep last 200 chars for patterns that span chunks
                searchBuffer = searchBuffer.substring(searchBuffer.length() - 200);
                
                Serial.println("Processed chunk. Current device count: " + String(deviceCount));
            }
            
            // Watchdog delay every 20KB
            if (totalBytesRead % 20480 == 0) {
                delay(1);
            }
        }
        
        Serial.println("Total bytes read: " + String(totalBytesRead));
        
        // Process any remaining buffer
        if (searchBuffer.length() > 0) {
            int idx = 0;
            while ((idx = searchBuffer.indexOf("\"mac\":\"", idx)) != -1) {
                deviceCount++;
                idx += 7;
            }
            
            idx = 0;
            while ((idx = searchBuffer.indexOf("\"is_wired\":true", idx)) != -1) {
                wiredCount++;
                idx += 15;
            }
            
            idx = 0;
            while ((idx = searchBuffer.indexOf("\"is_wired\":false", idx)) != -1) {
                wirelessCount++;
                idx += 16;
            }
        }
        
        delete[] chunk;  // Free heap memory
        
        Serial.println("*** DEVICE BREAKDOWN ***");
        Serial.println("  Total MACs found: " + String(deviceCount));
        Serial.println("  Wired: " + String(wiredCount));
        Serial.println("  Wireless: " + String(wirelessCount));
        Serial.println("*** SETTING DEVICE COUNT TO: " + String(deviceCount) + " ***");
        
        stats.connected_devices = deviceCount;
        stats.wired_devices = wiredCount;
        stats.wireless_devices = wirelessCount;
    } else {
        Serial.println("Client API HTTP error: " + String(httpCode));
        if (httpCode == 401) {
            Serial.println("Authentication failed - need to re-authenticate");
            // Try to re-authenticate
            if (authenticate()) {
                Serial.println("Re-authenticated successfully, retrying...");
                // Retry the request once
                https.end();
                return getClientStats(stats);
            }
        }
    }
    
    https.end();
}

// Get system statistics
void UniFiAPI::getSystemStats(NetworkStats& stats) {
    HTTPClient https;
    String sysinfoUrl = baseUrl + UNIFI_API_SYSINFO;
    
    #if DEBUG_API
    Serial.println("Fetching system stats from: " + sysinfoUrl);
    #endif
    
    if (!https.begin(client, sysinfoUrl)) {
        return;
    }
    
    // Always add authentication headers
    if (!sessionCookie.isEmpty()) {
        https.addHeader("Cookie", sessionCookie);
    }
    if (!csrfToken.isEmpty()) {
        https.addHeader("X-Csrf-Token", csrfToken);
    }
    #if DEBUG_API
    if (!sessionCookie.isEmpty()) {
        Serial.println("Using auth cookie: " + sessionCookie.substring(0, 40) + "...");
    }
    #endif
    
    int httpCode = https.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        String response = https.getString();
        
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, response);
        
        if (!error && doc["meta"]["rc"] == "ok") {
            JsonArray data = doc["data"];
            
            if (data.size() > 0) {
                JsonObject sysinfo = data[0];
                
                stats.wan_ip = sysinfo["wan_ip"] | "";
                stats.uptime_seconds = sysinfo["uptime"] | 0;
                
                #if DEBUG_API
                Serial.println("System info:");
                Serial.println("  WAN IP: " + stats.wan_ip);
                Serial.println("  Uptime: " + String(stats.uptime_seconds) + " seconds");
                #endif
            }
        }
    }
    
    https.end();
}

// Get network experience score from client and device satisfaction
void UniFiAPI::getNetworkExperience(NetworkStats& stats) {
    HTTPClient https;
    String clientsUrl = baseUrl + "/proxy/network/api/s/default/stat/sta";
    
    #if DEBUG_API
    Serial.println("Fetching network experience from clients: " + clientsUrl);
    #endif
    
    if (!https.begin(client, clientsUrl)) {
        return;
    }
    
    // Add authentication headers
    if (!sessionCookie.isEmpty()) {
        https.addHeader("Cookie", sessionCookie);
    }
    if (!csrfToken.isEmpty()) {
        https.addHeader("X-Csrf-Token", csrfToken);
    }
    
    int httpCode = https.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        String response = https.getString();
        
        DynamicJsonDocument doc(32768);  // 32KB for client data
        DeserializationError error = deserializeJson(doc, response);
        
        if (!error && doc["meta"]["rc"] == "ok") {
            JsonArray clients = doc["data"];
            
            float totalSatisfaction = 0;
            int satisfactionCount = 0;
            
            // Calculate average client satisfaction
            for (JsonObject client : clients) {
                if (client["satisfaction"]) {
                    float satisfaction = client["satisfaction"].as<float>();
                    if (satisfaction >= 0) {  // Valid satisfaction score
                        totalSatisfaction += satisfaction;
                        satisfactionCount++;
                    }
                }
            }
            
            if (satisfactionCount > 0) {
                stats.experience_score = totalSatisfaction / satisfactionCount;
                stats.data_source = "Experience";
                
                #if DEBUG_API
                Serial.printf("Client Experience: %.1f%% (from %d clients)\n", stats.experience_score, satisfactionCount);
                #endif
            } else {
                // Calculate a reasonable experience score based on network health
                float healthScore = 0;
                if (stats.ping_ms > 0 && stats.ping_ms < 50) {
                    healthScore += 40; // Good ping
                } else if (stats.ping_ms < 100) {
                    healthScore += 20; // OK ping
                }
                
                if (stats.uptime_percent > 99) {
                    healthScore += 40; // Excellent uptime
                } else if (stats.uptime_percent > 95) {
                    healthScore += 30; // Good uptime
                }
                
                if (stats.connected_devices > 0) {
                    healthScore += 20; // Devices connected
                }
                
                stats.experience_score = healthScore;
                stats.data_source = "Calculated";
                
                #if DEBUG_API
                Serial.printf("Calculated experience: %.1f%% (ping: %.1f, uptime: %.1f%%)\n", 
                             stats.experience_score, stats.ping_ms, stats.uptime_percent);
                #endif
            }
        }
    }
    
    https.end();
    
    // If we didn't get client satisfaction, try device satisfaction
    if (stats.experience_score == 0) {
        String devicesUrl = baseUrl + UNIFI_API_DEVICES;
        
        if (https.begin(client, devicesUrl)) {
            if (!sessionCookie.isEmpty()) {
                https.addHeader("Cookie", sessionCookie);
            }
            if (!csrfToken.isEmpty()) {
                https.addHeader("X-Csrf-Token", csrfToken);
            }
            
            httpCode = https.GET();
            
            if (httpCode == HTTP_CODE_OK) {
                String response = https.getString();
                
                DynamicJsonDocument doc(16384);  // 16KB for device data
                DeserializationError error = deserializeJson(doc, response);
                
                if (!error && doc["meta"]["rc"] == "ok") {
                    JsonArray devices = doc["data"];
                    
                    float totalSatisfaction = 0;
                    int satisfactionCount = 0;
                    
                    for (JsonObject device : devices) {
                        if (device["satisfaction"]) {
                            float satisfaction = device["satisfaction"].as<float>();
                            if (satisfaction >= 0) {  // Valid satisfaction score
                                totalSatisfaction += satisfaction;
                                satisfactionCount++;
                            }
                        }
                    }
                    
                    if (satisfactionCount > 0) {
                        stats.experience_score = totalSatisfaction / satisfactionCount;
                        stats.data_source = "Experience";
                        
                        #if DEBUG_API
                        Serial.printf("Device Experience: %.1f%% (from %d devices)\n", stats.experience_score, satisfactionCount);
                        #endif
                    }
                }
            }
            
            https.end();
        }
    }
}

// Get device statistics (gateway, switches, APs)
void UniFiAPI::getDeviceStats(NetworkStats& stats) {
    HTTPClient https;
    String devicesUrl = baseUrl + UNIFI_API_DEVICES;
    
    #if DEBUG_API
    Serial.println("Fetching device stats from: " + devicesUrl);
    #endif
    
    if (!https.begin(client, devicesUrl)) {
        return;
    }
    
    // Always add authentication headers
    if (!sessionCookie.isEmpty()) {
        https.addHeader("Cookie", sessionCookie);
    }
    if (!csrfToken.isEmpty()) {
        https.addHeader("X-Csrf-Token", csrfToken);
    }
    #if DEBUG_API
    if (!sessionCookie.isEmpty()) {
        Serial.println("Using auth cookie: " + sessionCookie.substring(0, 40) + "...");
    }
    #endif
    
    int httpCode = https.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        String response = https.getString();
        
        #if DEBUG_API
        Serial.println("Devices response length: " + String(response.length()));
        if (response.length() < 2000) {
            Serial.println("Full device response: " + response);
        }
        #endif
        
        // Use larger buffer for device stats too
        DynamicJsonDocument doc(65536);
        DeserializationError error = deserializeJson(doc, response);
        
        if (!error && doc["meta"]["rc"] == "ok") {
            JsonArray devices = doc["data"];
            
            #if DEBUG_API
            Serial.println("Found " + String(devices.size()) + " devices");
            #endif
            
            for (JsonObject device : devices) {
                String type = device["type"] | "";
                String name = device["name"] | "Unknown";
                
                #if DEBUG_API
                Serial.println("Device: " + name + " (type: " + type + ")");
                #endif
                
                // Look for gateway device (UDM, USG, etc)
                if (type == "ugw" || type == "udm" || type == "usg" || name.indexOf("Dream Machine") >= 0) {
                    // Try to get uplink stats
                    if (device["uplink"]) {
                        JsonObject uplink = device["uplink"];
                        
                        // Look for speed fields
                        if (uplink["speed"]) {
                            int speed = uplink["speed"];
                            #if DEBUG_API
                            Serial.println("  Uplink speed: " + String(speed) + " Mbps");
                            #endif
                        }
                        
                        stats.ping_ms = uplink["latency"] | 0.0f;
                        
                        // Get throughput
                        if (uplink["rx_bytes"]) {
                            unsigned long rx = uplink["rx_bytes"];
                            if (uplink["uptime"]) {
                                int uptime = uplink["uptime"];
                                if (uptime > 0) {
                                    stats.download_mbps = (rx * 8.0) / (uptime * 1000000.0);
                                }
                            }
                        }
                    }
                    
                    // Get total bytes from device for monthly usage
                    // Check for the 'bytes' field first (total)
                    if (device["bytes"]) {
                        unsigned long total = device["bytes"].as<unsigned long>();
                        stats.total_bytes_month = total;
                        stats.data_source = "Total";
                        #if DEBUG_API
                        Serial.printf("  Device total bytes: %lu (%.2f GB)\n", total, total / (1024.0*1024.0*1024.0));
                        #endif
                    } 
                    // Otherwise use rx_bytes + tx_bytes
                    else if (device["rx_bytes"] || device["tx_bytes"]) {
                        unsigned long rx = device["rx_bytes"] | 0UL;
                        unsigned long tx = device["tx_bytes"] | 0UL;
                        stats.total_bytes_month = rx + tx;
                        
                        // Set label based on what data we have
                        if (rx > 0 && tx > 0) {
                            stats.data_source = "RX+TX";
                        } else if (rx > 0) {
                            stats.data_source = "RX";
                        } else {
                            stats.data_source = "TX";
                        }
                        
                        #if DEBUG_API
                        Serial.printf("  Device RX: %.2f GB, TX: %.2f GB, Total: %.2f GB\n", 
                                     rx / (1024.0*1024.0*1024.0),
                                     tx / (1024.0*1024.0*1024.0),
                                     (rx + tx) / (1024.0*1024.0*1024.0));
                        #endif
                    }
                    
                    // Try port_table for more detailed stats
                    if (device["port_table"]) {
                        JsonArray ports = device["port_table"];
                        for (JsonObject port : ports) {
                            String port_name = port["name"] | "";
                            if (port_name == "wan" || port_name == "eth4" || port_name == "eth0") {
                                unsigned long rx_bytes = port["rx_bytes"] | 0;
                                unsigned long tx_bytes = port["tx_bytes"] | 0;
                                
                                #if DEBUG_API
                                Serial.println("  WAN port stats:");
                                Serial.println("    RX bytes: " + String(rx_bytes));
                                Serial.println("    TX bytes: " + String(tx_bytes));
                                #endif
                                
                                // Calculate rate if we have uptime
                                if (port["up"]) {
                                    // Port is up, use recent rate if available
                                    if (port["rx_rate"]) {
                                        stats.download_mbps = (port["rx_rate"].as<float>() * 8.0) / 1000000.0;
                                    }
                                    if (port["tx_rate"]) {
                                        stats.upload_mbps = (port["tx_rate"].as<float>() * 8.0) / 1000000.0;
                                    }
                                }
                            }
                        }
                    }
                    
                    // Check if device is online
                    int state = device["state"] | 0;
                    stats.wan_status = (state == 1) ? "CONNECTED" : "DISCONNECTED";
                    
                    #if DEBUG_API
                    Serial.println("  Gateway state: " + stats.wan_status);
                    #endif
                }
            }
        }
    } else {
        #if DEBUG_API
        Serial.println("Device API error: " + String(httpCode));
        #endif
    }
    
    https.end();
    
    // Try health endpoint as fallback
    if (stats.download_mbps == 0 && stats.upload_mbps == 0) {
        HTTPClient https2;
        String healthUrl = baseUrl + UNIFI_API_HEALTH;
        
        if (https2.begin(client, healthUrl)) {
            https2.addHeader("Cookie", sessionCookie);
            if (!csrfToken.isEmpty()) {
                https2.addHeader("X-CSRF-Token", csrfToken);
            }
            
            int code = https2.GET();
            if (code == HTTP_CODE_OK) {
                String resp = https2.getString();
                
                #if DEBUG_API
                Serial.println("Health endpoint checking for WAN stats...");
                #endif
                
                // Parse health data for throughput
                DynamicJsonDocument hdoc(16384);  // 16KB for health data
                if (!deserializeJson(hdoc, resp) && hdoc["meta"]["rc"] == "ok") {
                    JsonArray health = hdoc["data"];
                    
                    // First pass - look for overall site satisfaction/experience
                    for (JsonObject h : health) {
                        String subsystem = h["subsystem"] | "";
                        
                        // Check for overall site health/experience
                        if (subsystem == "" || subsystem == "site") {
                            if (h["satisfaction"]) {
                                stats.experience_score = h["satisfaction"].as<float>();
                                #if DEBUG_API
                                Serial.println("  Site satisfaction: " + String(stats.experience_score));
                                #endif
                            }
                        }
                    }
                    
                    // Second pass - get specific subsystem data
                    for (JsonObject h : health) {
                        String subsystem = h["subsystem"] | "";
                        
                        if (subsystem == "wan") {
                            // rx_bytes-r and tx_bytes-r are bytes per second
                            if (h["rx_bytes-r"]) {
                                float rx_bytes_per_sec = h["rx_bytes-r"].as<float>();
                                stats.download_mbps = (rx_bytes_per_sec * 8.0) / 1000000.0;
                                #if DEBUG_API
                                Serial.printf("  WAN rx_bytes-r: %.0f bytes/sec = %.2f Mbps\n", rx_bytes_per_sec, stats.download_mbps);
                                #endif
                            }
                            if (h["tx_bytes-r"]) {
                                float tx_bytes_per_sec = h["tx_bytes-r"].as<float>();
                                stats.upload_mbps = (tx_bytes_per_sec * 8.0) / 1000000.0;
                                #if DEBUG_API
                                Serial.printf("  WAN tx_bytes-r: %.0f bytes/sec = %.2f Mbps\n", tx_bytes_per_sec, stats.upload_mbps);
                                #endif
                            }
                            
                            stats.wan_status = h["status"] | "UNKNOWN";
                            
                            // Get latency and availability from uptime_stats
                            if (h["uptime_stats"]) {
                                JsonObject uptime_stats = h["uptime_stats"];
                                if (uptime_stats["WAN"]) {
                                    JsonObject wan = uptime_stats["WAN"];
                                    if (wan["latency_average"]) {
                                        stats.ping_ms = wan["latency_average"].as<float>();
                                        #if DEBUG_API
                                        Serial.println("  Got WAN latency from uptime_stats: " + String(stats.ping_ms) + " ms");
                                        #endif
                                    }
                                    // Get WAN availability as uptime percentage
                                    if (wan["availability"]) {
                                        stats.uptime_percent = wan["availability"].as<float>();
                                        #if DEBUG_API
                                        Serial.println("  WAN availability: " + String(stats.uptime_percent) + "%");
                                        #endif
                                    }
                                }
                            }
                            
                            // Get experience score if available
                            if (h["satisfaction"]) {
                                stats.experience_score = h["satisfaction"].as<float>();
                                #if DEBUG_API
                                Serial.println("  Experience score: " + String(stats.experience_score));
                                #endif
                            }
                            // Alternative field for experience
                            else if (h["xp_score"]) {
                                stats.experience_score = h["xp_score"].as<float>();
                                #if DEBUG_API
                                Serial.println("  XP score: " + String(stats.experience_score));
                                #endif
                            }
                            
                            // Get monthly data usage from WAN stats (keeping for potential use)
                            if (h["wan-rx_bytes"]) {
                                stats.total_bytes_month += h["wan-rx_bytes"].as<unsigned long>();
                            }
                            if (h["wan-tx_bytes"]) {
                                stats.total_bytes_month += h["wan-tx_bytes"].as<unsigned long>();
                            }
                            
                            #if DEBUG_API
                            Serial.println("  WAN status: " + stats.wan_status);
                            Serial.println("  WAN latency: " + String(stats.ping_ms) + " ms");
                            if (stats.experience_score > 0) {
                                Serial.println("  Experience: " + String(stats.experience_score) + "/100");
                            }
                            #endif
                        }
                        
                        if (subsystem == "wlan") {
                            int wlan_users = 0;
                            if (h["num_sta"]) {
                                wlan_users = h["num_sta"].as<int>();
                            } else if (h["num_user"]) {
                                wlan_users = h["num_user"].as<int>();
                            }
                            
                            if (wlan_users > 0) {
                                // Add WLAN users to total if we haven't counted clients yet
                                if (stats.connected_devices == 0 || stats.connected_devices < wlan_users) {
                                    stats.connected_devices = wlan_users;
                                }
                                #if DEBUG_API
                                Serial.println("  WLAN users: " + String(wlan_users));
                                #endif
                            }
                        }
                    }
                }
            }
            https2.end();
        }
    }
}

int UniFiAPI::getActiveClients() {
    if (!isConnected()) {
        return 0;
    }
    
    HTTPClient https;
    String clientsUrl = baseUrl + UNIFI_API_CLIENTS;
    
    if (!https.begin(client, clientsUrl)) {
        return 0;
    }
    
    // Always add authentication headers
    if (!sessionCookie.isEmpty()) {
        https.addHeader("Cookie", sessionCookie);
    }
    if (!csrfToken.isEmpty()) {
        https.addHeader("X-Csrf-Token", csrfToken);
    }
    #if DEBUG_API
    if (!sessionCookie.isEmpty()) {
        Serial.println("Using auth cookie: " + sessionCookie.substring(0, 40) + "...");
    }
    #endif
    
    int httpCode = https.GET();
    int clientCount = 0;
    
    if (httpCode == HTTP_CODE_OK) {
        String response = https.getString();
        
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, response);
        
        if (!error && doc["meta"]["rc"] == "ok") {
            JsonArray data = doc["data"];
            clientCount = data.size();
        }
    }
    
    https.end();
    return clientCount;
}

String UniFiAPI::extractCookie(String headers) {
    // UDM uses TOKEN cookie
    int tokenStart = headers.indexOf("TOKEN=");
    if (tokenStart != -1) {
        int tokenEnd = headers.indexOf(";", tokenStart);
        if (tokenEnd == -1) {
            tokenEnd = headers.length();
        }
        return headers.substring(tokenStart, tokenEnd);
    }
    
    // Some versions use unifises
    int sesStart = headers.indexOf("unifises=");
    if (sesStart != -1) {
        int sesEnd = headers.indexOf(";", sesStart);
        if (sesEnd == -1) {
            sesEnd = headers.length();
        }
        return headers.substring(sesStart, sesEnd);
    }
    
    // Try to get any cookie that looks like a session
    if (headers.indexOf("=") != -1) {
        // Just return the whole cookie string for now
        int end = headers.indexOf(";");
        if (end == -1) end = headers.length();
        return headers.substring(0, end);
    }
    
    return "";
}

String UniFiAPI::extractCSRF(String headers) {
    int csrfStart = headers.indexOf("X-CSRF-Token:");
    if (csrfStart != -1) {
        csrfStart += 14;  // Length of "X-CSRF-Token: "
        int csrfEnd = headers.indexOf("\r", csrfStart);
        if (csrfEnd != -1) {
            return headers.substring(csrfStart, csrfEnd);
        }
    }
    return "";
}

void UniFiAPI::setInsecure(bool insecure) {
    if (insecure) {
        client.setInsecure();
    }
}

bool UniFiAPI::testConnection() {
    return authenticate();
}