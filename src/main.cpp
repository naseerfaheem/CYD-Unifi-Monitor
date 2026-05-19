// Working UniFi Dashboard with prioritized touch handling
#include <Arduino.h>
#include <WiFi.h>
#include <TFT_eSPI.h>
#include <vector>
#include "config.h"
#include "credentials.h"
#include "unifi_api.h"
#include "network_handler.h"

// Simple approach: Touch priority, minimal API blocking

// Display and data
TFT_eSPI tft = TFT_eSPI();
UniFiAPI unifiAPI;
NetworkHandler network;
NetworkStats currentStats;

// Smooth animation variables
float smoothDownload = 0;
float smoothUpload = 0;
float targetDownload = 0;
float targetUpload = 0;
int targetDevices = 0;
float targetPing = 0;

// Device count smoothing
int deviceCountHistory[5] = {0, 0, 0, 0, 0};
int deviceHistoryIndex = 0;

// Graph data storage
#define GRAPH_POINTS 60
float downloadHistory[GRAPH_POINTS] = {0};
float uploadHistory[GRAPH_POINTS] = {0};
int graphScrollCounter = 0;
float graphMaxScale = 10.0;

// Display layout
#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 320
#define HEADER_HEIGHT 40
#define TAB_BAR_HEIGHT 30
#define TAB_BAR_Y HEADER_HEIGHT
#define CONTENT_START_Y (HEADER_HEIGHT + TAB_BAR_HEIGHT)
#define GRAPH_X 10
#define GRAPH_Y (CONTENT_START_Y + 10)
#define GRAPH_WIDTH 280
#define GRAPH_HEIGHT 135
#define SPEED_BOX_X 300
#define SPEED_BOX_Y (CONTENT_START_Y + 10)
#define SPEED_BOX_WIDTH 170
#define SPEED_BOX_HEIGHT 135
#define STATS_Y (CONTENT_START_Y + 155)
#define STATS_HEIGHT 95

// Dark Theme Colors (explicit color values for true dark theme)
#define COLOR_BG 0x0000              // Pure black background (explicit)
#define COLOR_PANEL_BG 0x2104        // Very dark grey panel background
#define COLOR_HEADER_START 0x0000    // Black header start (explicit)
#define COLOR_HEADER_END 0x0000      // Black header end (explicit)
#define COLOR_BORDER TFT_DARKGREY    // Dark gray border
#define COLOR_GRID TFT_DARKGREY      // Dark gray grid
#define COLOR_TEXT_PRIMARY TFT_WHITE // White text
#define COLOR_TEXT_SECONDARY TFT_LIGHTGREY // Light gray text
#define COLOR_DOWNLOAD 0x5D9F       // Turquoise/Cornflower blue for download
#define COLOR_UPLOAD 0xC11F         // Purple for upload
#define COLOR_SUCCESS TFT_GREEN      // Green for success
#define COLOR_STAT_VALUE TFT_CYAN    // Cyan for stat values

// Timing
unsigned long lastApiUpdate = 0;
unsigned long lastGraphScroll = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastStatsRedraw = 0;

const unsigned long API_INTERVAL = 2000;    // 2 seconds - fast bandwidth updates without device counting
const unsigned long GRAPH_SCROLL_INTERVAL = 1000;   // Slower graph scroll for touch responsiveness
const unsigned long DISPLAY_UPDATE_INTERVAL = 500;   // Less frequent display updates
const unsigned long STATS_REDRAW_INTERVAL = 2000;   // Much slower stats refresh for touch priority

// Function declarations
void drawHeader();
void drawStaticElements();
void updateSpeedDisplay();
void updateGraphDisplay();
void scrollGraphData();
void updateStatsBar();
void drawTabBar();
void handleTouch();
void drawTab1Content();
void drawTab2Content();
void drawTab3Content();

// Previous values for change detection
float prevDrawnDownload = -1;
float prevDrawnUpload = -1;
int prevDeviceCount = -1;
float prevPing = -1;

// Tab management
int currentTab = 1;
int prevTab = 0;
bool tabNeedsRedraw = true;

// Touch state - NON-BLOCKING
unsigned long lastTouchTime = 0;
unsigned long touchDebounce = 300;  // Increased debounce
int touchYOffset = 25;  // Y-axis calibration offset
bool touchPressed = false;
uint16_t lastTouchX = 0;
uint16_t lastTouchY = 0;

// Device list for tab 2
struct DeviceInfo {
    String name;
    String ip;
    bool online;
};
std::vector<DeviceInfo> deviceList;

// Removed dual-core complexity - using simple prioritized approach

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n=======================");
    Serial.println("UniFi Network Monitor");
    Serial.println("=======================");
    
    // Initialize display
    pinMode(27, OUTPUT);
    digitalWrite(27, HIGH);  // Backlight ON
    
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(0x0000);  // Explicit black color
    
    // Test touch initialization
    Serial.println("Testing touch hardware...");
    uint16_t testX, testY;
    bool touchAvailable = tft.getTouch(&testX, &testY);
    Serial.print("Initial touch test: ");
    Serial.println(touchAvailable ? "DETECTED" : "NO_TOUCH");
    Serial.println("Touch initialized with Y offset!");
    
    // Splash screen
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(3);
    tft.setCursor(100, 100);
    tft.print("UniFi Monitor");
    tft.setTextSize(2);
    tft.setCursor(150, 150);
    tft.print("Connecting...");
    
    // Connect to WiFi
    Serial.print("Connecting to WiFi");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 60) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected!");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
        
        // Connect to UniFi
        String unifiUrl = "https://" + String(UNIFI_HOST) + ":" + String(UNIFI_PORT);
        if (unifiAPI.connect(unifiUrl, UNIFI_USERNAME, UNIFI_PASSWORD)) {
            Serial.println("UniFi connected!");
        } else {
            Serial.println("UniFi connection failed!");
        }
    } else {
        Serial.println("\nWiFi connection failed!");
    }
    
    // Clear screen and draw UI - ensure true black background
    tft.fillScreen(0x0000);  // Use explicit black color value
    drawHeader();
    drawTabBar();
    drawStaticElements();
    
    // Simple single-threaded approach with touch priority
}

void loop() {
    unsigned long now = millis();
    
    // Handle touch with MAXIMUM PRIORITY - check every loop
    handleTouch();
    
    // Additional touch checks for ultra responsiveness  
    static unsigned long lastQuickTouch = 0;
    if (now - lastQuickTouch >= 1) {  // Check every 1ms for ULTRA responsiveness
        lastQuickTouch = now;
        handleTouch();
    }
    
    // Check if tab changed
    if (currentTab != prevTab || tabNeedsRedraw) {
        prevTab = currentTab;
        tabNeedsRedraw = false;
        
        // Clear content area
        tft.fillRect(0, CONTENT_START_Y, SCREEN_WIDTH, SCREEN_HEIGHT - CONTENT_START_Y, COLOR_BG);
        
        // Draw appropriate tab content
        if (currentTab == 1) {
            drawStaticElements();
        } else if (currentTab == 2) {
            drawTab2Content();
        } else if (currentTab == 3) {
            drawTab3Content();
        }
    }
    
    // Tab 1: Overview - Continue normal updates
    if (currentTab == 1) {
        // CRITICAL: Check touch again for Overview tab since it has heavy updates
        handleTouch();

        // Get new data from API - ULTRA FAST, NON-BLOCKING approach
        if (now - lastApiUpdate >= API_INTERVAL) {
            lastApiUpdate = now;
            
            if (unifiAPI.isConnected()) {
                // REAL API: Single fast call for bandwidth data
                Serial.printf("Real API call at %lums\n", now);
                
                // Get real network stats with fast timeout
                NetworkStats newStats = unifiAPI.getNetworkStats();
                
                // Only update if we got valid data
                if (newStats.download_mbps > 0 || newStats.upload_mbps > 0) {
                    currentStats = newStats;
                    
                    // Update targets for smooth animation  
                    targetDownload = currentStats.download_mbps;
                    targetUpload = currentStats.upload_mbps;
                    targetDevices = currentStats.connected_devices;
                    targetPing = currentStats.ping_ms;
                    
                    Serial.printf("REAL DATA: %d devices, %.1f/%.1f Mbps, %.1fms ping, %.0f%% exp\n", 
                                 currentStats.connected_devices, currentStats.download_mbps, 
                                 currentStats.upload_mbps, currentStats.ping_ms, currentStats.experience_score);
                } else {
                    Serial.println("API call failed or returned no data");
                }
            }
        }
        
        // Smooth animation
        if (now - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
            lastDisplayUpdate = now;
            
            float smoothingFactor = 0.15;
            smoothDownload += (targetDownload - smoothDownload) * smoothingFactor;
            smoothUpload += (targetUpload - smoothUpload) * smoothingFactor;
            
            updateSpeedDisplay();
        }
        
        // Scroll graph
        if (now - lastGraphScroll >= GRAPH_SCROLL_INTERVAL) {
            lastGraphScroll = now;
            scrollGraphData();
            updateGraphDisplay();
        }
        
        // Update stats bar
        if (now - lastStatsRedraw >= STATS_REDRAW_INTERVAL) {
            lastStatsRedraw = now;
            updateStatsBar();
        }
    }
    
    delay(1);  // ULTRA fast loop for maximum touch responsiveness
}

void handleTouch() {
    uint16_t x = 0, y = 0;
    
    // Check for touch
    bool pressed = tft.getTouch(&x, &y);
    unsigned long now = millis();
    
    // Debug: Report touch status periodically
    static unsigned long lastTouchReport = 0;
    if (now - lastTouchReport >= 1000) {  // Every second
        lastTouchReport = now;
        Serial.print("Touch status check: ");
        Serial.println(pressed ? "ACTIVE" : "inactive");
    }
    
    // Debug: Show raw touch status every few seconds
    static unsigned long lastDebug = 0;
    if (now - lastDebug > 5000) {  // Every 5 seconds
        Serial.print("Touch status check: ");
        Serial.println(pressed ? "ACTIVE" : "inactive");
        lastDebug = now;
    }
    
    // Apply Y calibration offset
    if (pressed) {
        // Touch at Y=278 needs to become Y=58 for tabs
        // Touch at Y=242 (tab area) needs to become Y=58
        // So we need to subtract about 184, not 220
        // Touch at Y=285 (tab area) needs to become Y=58 for tab switching
        // Fine-tune based on working touches at Y=40-43
        if (y > 200) {
            y = y - 225;  // Reduce offset - was too high
        } else {
            y = y + 12;   // Small adjustment for upper touches
        }
        
        Serial.print("Touch DOWN at X=");
        Serial.print(x);
        Serial.print(", Y=");
        Serial.print(y);
        Serial.println(" (adjusted)");
    }
    
    // Reset touch state if no touch for a while
    if (!pressed && (now - lastTouchTime > 300)) {
        touchPressed = false;
    }
    
    // Handle touch press with proper debouncing
    if (pressed && !touchPressed && (now - lastTouchTime > touchDebounce)) {
        touchPressed = true;
        lastTouchTime = now;
        
        // Check if touch is in tab bar area
        if (y >= TAB_BAR_Y && y <= TAB_BAR_Y + TAB_BAR_HEIGHT) {
            // Check which tab was pressed
            int tabWidth = SCREEN_WIDTH / 3;
            int newTab = (x / tabWidth) + 1;
            
            if (newTab >= 1 && newTab <= 3 && newTab != currentTab) {
                Serial.printf("Tab %d pressed!\n", newTab);
                currentTab = newTab;
                tabNeedsRedraw = true;
                drawTabBar();  // Redraw tab bar immediately
            }
        }
    } else if (!pressed && touchPressed) {
        // Touch release
        touchPressed = false;
        Serial.println("Touch UP");
    }
}

void drawHeader() {
    // Draw gradient header
    for (int y = 0; y < HEADER_HEIGHT; y++) {
        uint16_t color = tft.alphaBlend(y * 255 / HEADER_HEIGHT, COLOR_HEADER_END, COLOR_HEADER_START);
        tft.drawFastHLine(0, y, SCREEN_WIDTH, color);
    }
    
    tft.drawLine(0, HEADER_HEIGHT, SCREEN_WIDTH, HEADER_HEIGHT, COLOR_BORDER);
    
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setTextSize(2);
    tft.setCursor(10, 12);
    tft.print("UDM Network Monitor");
    
    // Status indicator
    int statusX = SCREEN_WIDTH - 120;
    tft.fillCircle(statusX, 20, 6, COLOR_SUCCESS);
    tft.setCursor(statusX + 15, 12);
    tft.print("Online");
}

void drawTabBar() {
    int tabWidth = SCREEN_WIDTH / 3;
    
    for (int i = 1; i <= 3; i++) {
        int x = (i - 1) * tabWidth;
        
        // Draw tab background
        if (i == currentTab) {
            tft.fillRect(x, TAB_BAR_Y, tabWidth, TAB_BAR_HEIGHT, 0x2104);
            tft.drawRect(x, TAB_BAR_Y, tabWidth, TAB_BAR_HEIGHT, COLOR_BORDER);
        } else {
            tft.fillRect(x, TAB_BAR_Y, tabWidth, TAB_BAR_HEIGHT, COLOR_BG);
            tft.drawRect(x, TAB_BAR_Y, tabWidth, TAB_BAR_HEIGHT, 0x2104);
        }
        
        // Draw tab text
        tft.setTextColor(i == currentTab ? TFT_WHITE : 0x8410);
        tft.setTextSize(2);
        
        String tabText;
        if (i == 1) tabText = "Overview";
        else if (i == 2) tabText = "Devices";
        else tabText = "Ports";
        
        int textWidth = tabText.length() * 12;
        tft.setCursor(x + (tabWidth - textWidth) / 2, TAB_BAR_Y + 7);
        tft.print(tabText);
    }
}

void drawStaticElements() {
    // Draw graph panel
    tft.drawRoundRect(GRAPH_X, GRAPH_Y, GRAPH_WIDTH, GRAPH_HEIGHT, 8, COLOR_BORDER);
    tft.fillRoundRect(GRAPH_X+1, GRAPH_Y+1, GRAPH_WIDTH-2, GRAPH_HEIGHT-2, 8, COLOR_PANEL_BG);
    
    // Graph title
    tft.setTextColor(COLOR_TEXT_SECONDARY);
    tft.setTextSize(1);
    tft.setCursor(GRAPH_X + 10, GRAPH_Y + 5);
    tft.print("Network Throughput (Mbps)");
    
    // Draw speed panel
    tft.drawRoundRect(SPEED_BOX_X, SPEED_BOX_Y, SPEED_BOX_WIDTH, SPEED_BOX_HEIGHT, 8, COLOR_BORDER);
    tft.fillRoundRect(SPEED_BOX_X+1, SPEED_BOX_Y+1, SPEED_BOX_WIDTH-2, SPEED_BOX_HEIGHT-2, 8, COLOR_PANEL_BG);
    
    // Draw stats panels
    for (int i = 0; i < 4; i++) {
        int x = i * (SCREEN_WIDTH / 4) + 5;
        tft.drawRoundRect(x, STATS_Y, (SCREEN_WIDTH / 4) - 10, STATS_HEIGHT - 10, 6, COLOR_BORDER);
        tft.fillRoundRect(x+1, STATS_Y+1, (SCREEN_WIDTH / 4) - 12, STATS_HEIGHT - 12, 6, COLOR_PANEL_BG);
    }
}

void updateSpeedDisplay() {
    // Only update if values changed significantly
    bool downloadChanged = abs(smoothDownload - prevDrawnDownload) >= 0.01;
    bool uploadChanged = abs(smoothUpload - prevDrawnUpload) >= 0.01;
    
    if (!downloadChanged && !uploadChanged) {
        return;
    }
    
    int centerX = SPEED_BOX_X + SPEED_BOX_WIDTH / 2;
    
    // Update speeds
    if (downloadChanged) {
        prevDrawnDownload = smoothDownload;
        
        tft.fillRect(SPEED_BOX_X + 10, SPEED_BOX_Y + 20, SPEED_BOX_WIDTH - 20, 35, COLOR_PANEL_BG);
        
        tft.setTextColor(COLOR_DOWNLOAD);
        tft.setTextSize(3);
        String downStr = String(smoothDownload, 1);
        tft.setCursor(centerX - downStr.length() * 9, SPEED_BOX_Y + 25);
        tft.print(downStr);
        
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.setTextSize(1);
        tft.setCursor(centerX - 35, SPEED_BOX_Y + 55);
        tft.print("Download Mbps");
    }
    
    if (uploadChanged) {
        prevDrawnUpload = smoothUpload;
        
        tft.fillRect(SPEED_BOX_X + 10, SPEED_BOX_Y + 80, SPEED_BOX_WIDTH - 20, 35, COLOR_PANEL_BG);
        
        tft.setTextColor(COLOR_UPLOAD);
        tft.setTextSize(3);
        String upStr = String(smoothUpload, 1);
        tft.setCursor(centerX - upStr.length() * 9, SPEED_BOX_Y + 85);
        tft.print(upStr);
        
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.setTextSize(1);
        tft.setCursor(centerX - 30, SPEED_BOX_Y + 115);
        tft.print("Upload Mbps");
    }
}

void updateGraphDisplay() {
    // Clear graph area
    int graphAreaY = GRAPH_Y + 20;
    int graphAreaHeight = GRAPH_HEIGHT - 25;
    tft.fillRect(GRAPH_X + 10, graphAreaY, GRAPH_WIDTH - 20, graphAreaHeight, COLOR_PANEL_BG);
    
    // Auto-scale
    float maxVal = 0.1;
    for (int i = 0; i < GRAPH_POINTS; i++) {
        if (downloadHistory[i] > maxVal) maxVal = downloadHistory[i];
        if (uploadHistory[i] > maxVal) maxVal = uploadHistory[i];
    }
    
    if (smoothDownload > maxVal) maxVal = smoothDownload;
    if (smoothUpload > maxVal) maxVal = smoothUpload;
    
    maxVal = maxVal * 1.3;
    if (maxVal < 0.5) maxVal = 0.5;
    
    // Draw grid
    for (int i = 0; i <= 4; i++) {
        int y = graphAreaY + (graphAreaHeight * i / 4);
        for (int x = GRAPH_X + 10; x < GRAPH_X + GRAPH_WIDTH - 10; x += 6) {
            tft.drawPixel(x, y, COLOR_GRID);
        }
    }
    
    // Draw curves
    for (int i = 1; i < GRAPH_POINTS; i++) {
        int x1 = GRAPH_X + 10 + ((i-1) * (GRAPH_WIDTH - 20) / GRAPH_POINTS);
        int x2 = GRAPH_X + 10 + (i * (GRAPH_WIDTH - 20) / GRAPH_POINTS);
        
        // Download line
        int y1 = graphAreaY + graphAreaHeight - (downloadHistory[i-1] * graphAreaHeight / maxVal);
        int y2 = graphAreaY + graphAreaHeight - (downloadHistory[i] * graphAreaHeight / maxVal);
        tft.drawLine(x1, y1, x2, y2, COLOR_DOWNLOAD);
        
        // Upload line
        y1 = graphAreaY + graphAreaHeight - (uploadHistory[i-1] * graphAreaHeight / maxVal);
        y2 = graphAreaY + graphAreaHeight - (uploadHistory[i] * graphAreaHeight / maxVal);
        tft.drawLine(x1, y1, x2, y2, COLOR_UPLOAD);
    }
}

void scrollGraphData() {
    // Shift data left
    for (int i = 0; i < GRAPH_POINTS - 1; i++) {
        downloadHistory[i] = downloadHistory[i + 1];
        uploadHistory[i] = uploadHistory[i + 1];
    }
    
    // Add new data
    downloadHistory[GRAPH_POINTS - 1] = smoothDownload;
    uploadHistory[GRAPH_POINTS - 1] = smoothUpload;
}

void updateStatsBar() {
    // Calculate smoothed device count
    int smoothedDevices = 0;
    int validCount = 0;
    for (int i = 0; i < 5; i++) {
        if (deviceCountHistory[i] > 0) {
            smoothedDevices += deviceCountHistory[i];
            validCount++;
        }
    }
    if (validCount > 0) {
        smoothedDevices = smoothedDevices / validCount;
    } else {
        smoothedDevices = currentStats.connected_devices;
    }
    
    // Update each stat
    for (int i = 0; i < 4; i++) {
        int x = i * (SCREEN_WIDTH / 4) + 5;
        
        // Clear value area
        tft.fillRect(x + 5, STATS_Y + 25, (SCREEN_WIDTH / 4) - 20, 35, COLOR_PANEL_BG);
        
        tft.setTextColor(COLOR_TEXT_SECONDARY);
        tft.setTextSize(1);
        tft.setCursor(x + 10, STATS_Y + 10);
        
        tft.setTextColor(COLOR_STAT_VALUE);
        tft.setTextSize(2);
        
        switch(i) {
            case 0:  // Monthly Data Usage
                tft.setTextColor(COLOR_TEXT_SECONDARY);
                tft.setTextSize(1);
                tft.setCursor(x + 10, STATS_Y + 10);
                tft.print("Monthly");
                tft.setTextColor(COLOR_STAT_VALUE);
                tft.setTextSize(2);
                tft.setCursor(x + 10, STATS_Y + 30);
                tft.print(String(currentStats.monthly_data_gb, 1));
                tft.setTextSize(1);
                tft.print(" GB");
                break;
                
            case 1:  // Ping
                tft.setTextColor(COLOR_TEXT_SECONDARY);
                tft.setTextSize(1);
                tft.setCursor(x + 10, STATS_Y + 10);
                tft.print("Ping");
                tft.setTextColor(COLOR_STAT_VALUE);
                tft.setTextSize(2);
                tft.setCursor(x + 10, STATS_Y + 30);
                tft.print(String(currentStats.ping_ms, 1));
                tft.setTextSize(1);
                tft.print(" ms");
                break;
                
            case 2: { // Network Experience Score
                tft.setTextColor(COLOR_TEXT_SECONDARY);
                tft.setTextSize(1);
                tft.setCursor(x + 10, STATS_Y + 10);
                tft.print("Experience");
                
                tft.setTextColor(COLOR_STAT_VALUE);
                tft.setTextSize(2);
                tft.setCursor(x + 10, STATS_Y + 30);
                
                // Show experience - use 100% as fallback for good network
                if (unifiAPI.isConnected()) {
                    if (currentStats.experience_score > 0) {
                        tft.print(currentStats.experience_score, 0);
                    } else {
                        tft.print("100");  // Default to 100% when connected
                    }
                    tft.print("%");
                } else {
                    tft.print("--");
                }
                break;
            }
                
            case 3:  // Uptime
                tft.setTextColor(COLOR_TEXT_SECONDARY);
                tft.setTextSize(1);
                tft.setCursor(x + 10, STATS_Y + 10);
                tft.print("Uptime");
                tft.setTextColor(COLOR_STAT_VALUE);
                tft.setTextSize(2);
                tft.setCursor(x + 10, STATS_Y + 30);
                tft.print(String(currentStats.uptime_percent, 1));
                tft.print("%");
                break;
        }
    }
}

void drawTab2Content() {
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setTextSize(2);
    tft.setCursor(10, CONTENT_START_Y + 10);
    tft.print("Network Devices");
    
    tft.setTextSize(1);
    tft.setCursor(10, CONTENT_START_Y + 40);
    tft.print("Total devices: ");
    tft.print(currentStats.connected_devices);
    
    // Device list would go here
    tft.setCursor(10, CONTENT_START_Y + 60);
    tft.print("Device list coming soon...");
}

void drawTab3Content() {
    tft.setTextColor(COLOR_TEXT_PRIMARY);
    tft.setTextSize(2);
    tft.setCursor(10, CONTENT_START_Y + 10);
    tft.print("Port Status");
    
    tft.setTextSize(1);
    tft.setCursor(10, CONTENT_START_Y + 40);
    tft.print("Port information coming soon...");
}