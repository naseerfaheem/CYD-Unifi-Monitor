// User_Setup.h for ESP32 CYD 4.0" with ST7796 driver

#ifndef USER_SETUP_H
#define USER_SETUP_H

// Driver selection
#define ST7796_DRIVER

// Display resolution
#define TFT_WIDTH  480
#define TFT_HEIGHT 320

// ESP32 CYD Pin connections - Display uses HSPI
// #define TFT_MISO 12   // DO NOT DEFINE - ST7796 doesn't release SPI bus!
#define TFT_MOSI 13   // SDI/MOSI
#define TFT_SCLK 14   // SCK
#define TFT_CS   15   // Chip select
#define TFT_DC    2   // Data/Command
#define TFT_RST  -1   // Reset pin not used (tied to EN)
#define TFT_BL   27   // Backlight control (pin 27, not 21!)

// Touch pins - CRITICAL: CYD 4.0" uses SEPARATE chip select!
#define TOUCH_CS 33   // Touch chip select pin - DIFFERENT from display!

// Use HSPI port for ESP32 - CRITICAL for touch to work!
#define USE_HSPI_PORT

// Define the SPI frequency
#define SPI_FREQUENCY  40000000  // 40MHz for display
#define SPI_READ_FREQUENCY  16000000  // For reading back

// Font support
#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
#define LOAD_FONT6  // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
#define LOAD_FONT7  // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:.
#define LOAD_FONT8  // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.
#define LOAD_GFXFF  // FreeFonts. Include access to the 48 Adafruit_GFX free fonts FF1 to FF48 and custom fonts

#define SMOOTH_FONT

// Other display settings
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000

#endif // USER_SETUP_H