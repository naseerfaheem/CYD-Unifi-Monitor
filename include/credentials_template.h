#ifndef CREDENTIALS_H
#define CREDENTIALS_H

// WiFi Configuration
#define WIFI_SSID "your-wifi-ssid"
#define WIFI_PASSWORD "your-wifi-password"

// UniFi Dream Machine Configuration
// IMPORTANT: This is your UniFi CONTROLLER IP, not necessarily your gateway IP!
// Your gateway is 192.168.1.1, but the UniFi controller might be different.
// See FIND_UNIFI_IP.md for help finding the correct IP
//
// To find your UniFi Controller IP:
// 1. Try https://192.168.1.1 in your browser (if UDM is your gateway)
// 2. Try https://unifi or https://unifi.local
// 3. Check your DHCP leases for "Dream Machine" or "UDM"
// 4. Use the UniFi mobile app - it shows the controller IP
#define UNIFI_HOST "192.168.1.1"      // Replace with YOUR UniFi Controller IP
#define UNIFI_PORT 443                  // HTTPS port (keep as 443)
#define UNIFI_USERNAME "your-local-user" // Your LOCAL UniFi username (NOT ui.com account!)
#define UNIFI_PASSWORD "your-password"   // Your LOCAL UniFi password

// Optional: Custom site name (keep as "default" for most users)
#define UNIFI_SITE "default"

// Keep this as true for self-signed certificates (required for local access)
#define UNIFI_SKIP_SSL_VERIFY true

// Optional: NTP Server for time sync
#define NTP_SERVER "pool.ntp.org"
#define TIMEZONE_OFFSET -5              // Your timezone offset from UTC

#endif // CREDENTIALS_H