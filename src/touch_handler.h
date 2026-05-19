#ifndef TOUCH_HANDLER_H
#define TOUCH_HANDLER_H

#include <Arduino.h>

// Touch pins for CYD 4.0"
#define TOUCH_CS_PIN   33
#define TOUCH_IRQ_PIN  36  
#define TOUCH_MOSI_PIN 32
#define TOUCH_MISO_PIN 39
#define TOUCH_CLK_PIN  25

class SimpleTouchHandler {
private:
    // Software SPI for touch
    void writeSPI(uint8_t data) {
        for(int i = 7; i >= 0; i--) {
            digitalWrite(TOUCH_CLK_PIN, LOW);
            digitalWrite(TOUCH_MOSI_PIN, (data >> i) & 0x01);
            delayMicroseconds(1);
            digitalWrite(TOUCH_CLK_PIN, HIGH);
            delayMicroseconds(1);
        }
    }
    
    uint16_t readSPI16() {
        uint16_t data = 0;
        for(int i = 0; i < 16; i++) {
            digitalWrite(TOUCH_CLK_PIN, LOW);
            delayMicroseconds(1);
            digitalWrite(TOUCH_CLK_PIN, HIGH);
            data = (data << 1) | digitalRead(TOUCH_MISO_PIN);
            delayMicroseconds(1);
        }
        return data;
    }

public:
    void begin() {
        pinMode(TOUCH_CS_PIN, OUTPUT);
        pinMode(TOUCH_CLK_PIN, OUTPUT);
        pinMode(TOUCH_MOSI_PIN, OUTPUT);
        pinMode(TOUCH_MISO_PIN, INPUT);
        pinMode(TOUCH_IRQ_PIN, INPUT);
        
        digitalWrite(TOUCH_CS_PIN, HIGH);
        digitalWrite(TOUCH_CLK_PIN, HIGH);
    }
    
    bool touched() {
        return digitalRead(TOUCH_IRQ_PIN) == LOW;
    }
    
    void readTouch(int &x, int &y, int &z) {
        digitalWrite(TOUCH_CS_PIN, LOW);
        
        // Read X
        writeSPI(0xD0);  // Start bit, A2-A0 = 001 (X), MODE = 0, SER/DFR = 0, PD1-0 = 00
        x = readSPI16() >> 3;
        
        // Read Y  
        writeSPI(0x90);  // Start bit, A2-A0 = 101 (Y), MODE = 0, SER/DFR = 0, PD1-0 = 00
        y = readSPI16() >> 3;
        
        // Read Z1 (pressure)
        writeSPI(0xB0);  // Start bit, A2-A0 = 011 (Z1), MODE = 0, SER/DFR = 0, PD1-0 = 00
        z = readSPI16() >> 3;
        
        digitalWrite(TOUCH_CS_PIN, HIGH);
    }
    
    bool getTouchPoint(int &screenX, int &screenY) {
        if (!touched()) {
            return false;
        }
        
        int x, y, z;
        readTouch(x, y, z);
        
        // Check for valid touch (z in reasonable range)
        if (z < 100 || z > 2000) {
            return false;
        }
        
        // Map to screen coordinates (calibrated for CYD 4.0")
        // These values may need adjustment
        screenX = map(x, 400, 3900, 0, 480);
        screenY = map(y, 400, 3900, 0, 320);
        
        // Constrain to screen bounds
        screenX = constrain(screenX, 0, 479);
        screenY = constrain(screenY, 0, 319);
        
        return true;
    }
};

#endif // TOUCH_HANDLER_H