#include "keyboard_interface.h"

void KeyboardInterface::begin() {
  Wire.begin();
    
  this->init_time = millis();
}

char KeyboardInterface::main(uint32_t current_time) {
  // Request data from I2C slave
  if (current_time - this->init_time >= KEYBOARD_INT) {
    Wire.requestFrom(I2C_ADDR, 1);
    while (Wire.available())
    {
      char c = Wire.read(); // receive a byte as character
      this->KEY = c;
      return c;
      
    }

    this->init_time = millis();
  }
}
