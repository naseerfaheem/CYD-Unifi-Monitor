# CYD UniFi Monitor

A small always-on display for your home network. It shows your current download and upload speeds, how many devices are connected, ping, and uptime, pulled live from your UniFi controller.

Built for the 4-inch ESP32-S3 CYD (the "Cheap Yellow Display"), which you can find on AliExpress or Amazon for around $20.

> This is a personal hobby project. It's not affiliated with Ubiquiti Inc. "UniFi" and "Dream Machine" are trademarks of Ubiquiti.

## What you'll need

- An ESP32-S3 CYD with a 4-inch 320x480 screen (the one with an ST7796 driver)
- A USB-C cable that supports data (not the cheap charge-only kind)
- A UniFi Dream Machine on your network (UDM, UDM Pro, or UDM SE)
- A computer with [VS Code](https://code.visualstudio.com) installed
- A 2.4 GHz Wi-Fi network (the ESP32 doesn't do 5 GHz)
- Optional: a 3D-printed case. I designed one for this exact board — [Desktop Case for CYD ESP32 4" Display on MakerWorld](https://makerworld.com/en/models/1810329-desktop-case-for-cyd-esp32-4-display#profileId-1942741).

## Setup

### 1. Install PlatformIO

Open VS Code, go to Extensions (Ctrl+Shift+X), search for "PlatformIO IDE", and install the official one. Restart VS Code when it asks.

### 2. Make a local user on UniFi

You don't want to give your microcontroller your main UniFi password. In the UniFi web interface, go to Settings → Admins & Users → Add Admin. Set the Account Type to **Local Access Only** and give it a password you're okay having sit in a file on your computer.

While you're there, note your controller's IP address (Settings → System → Advanced). If you can't find it, see [FIND_UNIFI_IP.md](FIND_UNIFI_IP.md).

### 3. Add your credentials

Clone or download this repo. In the `include/` folder, copy `credentials_template.h` to `credentials.h` and fill in the blanks:

```cpp
#define WIFI_SSID      "your-wifi-name"
#define WIFI_PASSWORD  "your-wifi-password"
#define UNIFI_HOST     "192.168.1.1"
#define UNIFI_USERNAME "the-local-user-you-just-made"
#define UNIFI_PASSWORD "its-password"
```

`credentials.h` is gitignored, so it stays on your machine.

### 4. Plug in the CYD

Connect the board to your computer with a USB-C cable.

On Windows you may need the [CP210x USB driver](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers) for the COM port to appear. After installing it, open Device Manager and look under "Ports (COM & LPT)" for something like COM3 or COM6.

Open `platformio.ini` and put your COM port on the `upload_port` line.

### 5. Build and upload

Click the PlatformIO icon (the alien head) in the sidebar, expand the project tasks, then click **Upload**. The first build downloads some libraries and can take a couple of minutes. When it's done, the CYD will reboot and start showing data within about 10 seconds.

## When something's wrong

**Screen is blank.** Try a different USB cable. The cheap charger-only ones have no data lines. If that doesn't help, hit the reset button on the back of the CYD.

**"WiFi Connection Failed".** Double-check the SSID and password. Make sure it's a 2.4 GHz network. Many modern routers have separate 2.4 and 5 GHz networks even when they share a name.

**"Authentication failed. HTTP code: 401".** The UniFi username or password is wrong, or the user isn't set to Local Access Only.

**All values are zero.** Wait 30 seconds. UniFi doesn't return throughput numbers right after login.

For more detail, open the PlatformIO serial monitor (115200 baud) and watch the boot output.

If you're stuck on something that isn't covered here, paste the error from the serial monitor or the PlatformIO build log into ChatGPT or Claude. They're surprisingly good at decoding ESP32 build errors and UniFi API responses.

## Customizing

The things you might want to tweak live in `include/config.h`: refresh intervals, screen brightness, colors.

## Helper scripts

The `tools/` folder has a few Python scripts that hit the same UniFi endpoints from your computer. Handy for testing whether your credentials work or for poking at the API. They read the same `include/credentials.h` file you already filled in.

```
cd cyd-unifi-monitor
python tools/test_auth.py
```

You'll need Python 3 and the `requests` library (`pip install requests`).

## License

MIT. See [LICENSE](LICENSE).
