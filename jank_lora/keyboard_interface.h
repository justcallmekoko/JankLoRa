#ifndef keyboard_interface_h
#define keyboard_interface_h

#include <Wire.h>
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define I2C_ADDR 0x08
#define KEYBOARD_INT 10

extern Adafruit_SSD1306 display;

class KeyboardInterface {
  private:
    uint32_t init_time;

  public:
    char KEY = NULL;
    
    void begin();
    char main(uint32_t current_time);
};

#endif
