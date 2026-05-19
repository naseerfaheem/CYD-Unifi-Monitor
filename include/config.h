#ifndef CONFIG_H
#define CONFIG_H

// Display Configuration
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 480
#define DISPLAY_ROTATION 0  // 0=portrait, 1=landscape

// Network Configuration
#define WIFI_CONNECT_TIMEOUT 10000  // 10 seconds
#define API_TIMEOUT 5000            // 5 seconds
#define API_RETRY_DELAY 30000       // 30 seconds after failure

// Update Intervals (milliseconds)
#define API_UPDATE_INTERVAL 5000    // Fetch new data every 5 seconds
#define CHART_UPDATE_INTERVAL 1000  // Update chart every second
#define UI_REFRESH_RATE 20          // 50 FPS (1000/20)

// Chart Configuration
#define CHART_HISTORY_POINTS 30     // 30 seconds of history (reduced for RAM)
#define CHART_Y_GRID_COUNT 4        // Horizontal grid lines
#define CHART_AUTO_SCALE true       // Auto-scale Y axis

// Data Smoothing
#define SPEED_SMOOTHING_FACTOR 0.3  // Lower = more smoothing
#define ENABLE_MOVING_AVERAGE true
#define MOVING_AVERAGE_WINDOW 3

// Memory Management
#define JSON_BUFFER_SIZE 8192       // Increased for UniFi API responses
#define MAX_DEVICES_TRACKED 50      // Maximum devices to track

// UI Theme Colors (RGB565 format helpers)
#define COLOR_BACKGROUND 0x0000     // Black
#define COLOR_PRIMARY 0x07FF        // Cyan
#define COLOR_SECONDARY 0xF800      // Red  
#define COLOR_SUCCESS 0x07E0        // Green
#define COLOR_WARNING 0xFFE0        // Yellow
#define COLOR_ERROR 0xF800          // Red
#define COLOR_TEXT 0xFFFF           // White
#define COLOR_TEXT_DIM 0x8410       // Gray

// Debug Flags
#define DEBUG_SERIAL 1              // Enable serial debugging
#define DEBUG_API 1                 // Log API calls and responses
#define DEBUG_UI 0                  // Log UI updates
#define DEBUG_MEMORY 0              // Monitor memory usage

// Feature Flags
#define ENABLE_TOUCH 1              // Enable touch input for future tab navigation
#define ENABLE_OTA 0                // Enable OTA updates
#define ENABLE_WEB_CONFIG 0         // Enable web configuration portal
#define ENABLE_SD_LOGGING 0         // Log data to SD card

// Power Management
#define ENABLE_AUTO_BRIGHTNESS 1    // Adjust based on time
#define BRIGHTNESS_DAY 255          // Full brightness
#define BRIGHTNESS_NIGHT 100        // Dimmed for night
#define NIGHT_HOUR_START 22         // 10 PM
#define NIGHT_HOUR_END 6            // 6 AM

// UniFi API Endpoints (UDM requires /proxy/network prefix)
#define UNIFI_API_LOGIN "/api/auth/login"
#define UNIFI_API_HEALTH "/proxy/network/api/s/default/stat/health"
#define UNIFI_API_SITES "/proxy/network/api/s/default/stat/sites"
#define UNIFI_API_DEVICES "/proxy/network/api/s/default/stat/device"
#define UNIFI_API_CLIENTS "/proxy/network/api/s/default/stat/sta"
#define UNIFI_API_SYSINFO "/proxy/network/api/s/default/stat/sysinfo"

// Error Messages
#define MSG_WIFI_CONNECTING "Connecting to WiFi..."
#define MSG_WIFI_FAILED "WiFi connection failed"
#define MSG_API_CONNECTING "Connecting to UniFi..."
#define MSG_API_FAILED "UniFi API error"
#define MSG_NO_DATA "No data available"

// Version
#define FIRMWARE_VERSION "1.0.0"
#define BUILD_DATE __DATE__
#define BUILD_TIME __TIME__

#endif // CONFIG_H