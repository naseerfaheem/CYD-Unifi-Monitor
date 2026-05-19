# How to Find Your UniFi Controller IP Address

Your UniFi Controller IP is NOT necessarily your gateway IP (192.168.1.1). The controller runs on a specific device.

## Method 1: Check Your Devices

### If you have a Dream Machine (UDM/UDM-Pro/UDM-SE):
- The controller IP **IS** the Dream Machine's IP
- This is often the gateway (192.168.1.1) but not always
- Try: `https://192.168.1.1`

### If you have a Cloud Key or Controller on another device:
- The controller IP is that device's IP address
- This will be different from your gateway

## Method 2: Use UniFi Discovery

1. **Download UniFi Network Application** on your phone:
   - iOS: App Store → "UniFi Network"
   - Android: Play Store → "UniFi Network"

2. **Open the app** while on your local network
3. It will show your controller's IP address

## Method 3: Check Your Router's DHCP Leases

1. Log into your router at `192.168.1.1`
2. Look for DHCP clients/leases
3. Find device named:
   - "Dream Machine"
   - "UDM"
   - "UniFi"
   - "Cloud Key"

## Method 4: Network Scan

From Windows PowerShell or Command Prompt:
```bash
# Try common UniFi ports
nmap -p 8443,443 192.168.1.0/24
```

Or try these common addresses in your browser:
- `https://192.168.1.1` (if UDM is your gateway)
- `https://unifi` (if mDNS is working)
- `https://unifi.local`
- `https://192.168.1.2` (common if UDM is not gateway)

## Method 5: Check UniFi Account

1. Go to https://unifi.ui.com
2. Log in with your Ubiquiti account
3. Your controller should be listed with its local IP

## Method 6: Direct Discovery Tool

1. Download **UniFi Network Application** for Windows/Mac
2. Run it and click "Manage Existing Network"
3. It will scan and show all UniFi controllers on your network

## Quick Test

Once you think you have the IP, test it:

1. Open a web browser
2. Navigate to: `https://YOUR_IP_HERE`
3. You should see the UniFi login page
4. If you get a certificate warning, that's normal - proceed anyway

## Common IPs to Try

Based on your network (192.168.1.x):
- `https://192.168.1.1` - If UDM is your gateway
- `https://192.168.1.2` - Common alternative
- `https://192.168.1.5` - Sometimes used
- `https://unifi` - mDNS name

## Still Can't Find It?

Run this from Command Prompt to find all devices with HTTPS:
```bash
for /L %i in (1,1,254) do @ping -n 1 -w 100 192.168.1.%i | find "Reply"
```

Then try HTTPS on each IP that responds.

## For Our Project

Once you find your UniFi Controller IP, update `credentials.h`:
```cpp
#define UNIFI_HOST "192.168.1.X"  // Replace X with actual number
```

The IP you need is specifically for the **UniFi Network Application/Controller**, not just your network gateway!