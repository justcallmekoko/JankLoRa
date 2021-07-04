#include "keyboard_interface.h"
#include "Display.h"

#include <SPI.h>
#include <Wire.h>
#include <RH_RF95.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED STUFF
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define LETTER_HEIGHT 8
#define LETTER_WIDTH  6

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int16_t last_cursor_x = 0;
int16_t last_cursor_y = 0;

// LORA STUFF
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 7

#define RF95_FREQ 915.0

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

// Blinky on receipt
#define LED 13

// MY STUFF

KeyboardInterface keyboard_obj;
Display display_obj;

uint32_t currentTime  = 0;

String received_message = "";

String send_message = "";

// END MY STUFF

void setup() {
  // LORA SETUP
  pinMode(LED, OUTPUT);
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    Serial.println("Uncomment '#define SERIAL_DEBUG' in RH_RF95.cpp for detailed debug info");
    while (1);
  }
  Serial.println("LoRa radio init OK!");

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);

  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);

  // END LORA SETUP

  // GENERAL SETUP
  
  Serial.begin(115200);

  delay(100);

  Serial.println("\n\nJank LoRa v0.1\n");
  
  keyboard_obj.begin();

  // END GENERAL SETUP

  // DISPLAY SETUP

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();
  display.display();

  //testDisplay();
  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, LETTER_HEIGHT);
  for (int i = 0; i < SCREEN_WIDTH / LETTER_WIDTH; i++) {
    display.print("-");
    display.display();
    delay(10);
  }
  display.setCursor(0, LETTER_HEIGHT * 2);
  last_cursor_x = display.getCursorX();
  last_cursor_y = display.getCursorY();

  // END DISPLAY SETUP
}

void testDisplay() {
  display.clearDisplay();
  display.display();
  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  display.println(F("Hello, world!\nTest"));
  display.display();
}

void handleKeyboardInput(char c) {
  if (c) {
    if (c == 0x18) {
      //Serial.println("\nUP");
    }
    else if (c == 0x19) {
      //Serial.println("\nDOWN");
    }
    else if (c == 0x1A) {
      //Serial.println("\nRIGHT");
    }
    else if (c == 0x1B) {
      //Serial.println("\nLEFT");
    }
    else if (c == 0x05) {
      //Serial.println("\nCENTER");
    }
    else if (c == '\n') {
      if (send_message != "") {
        //Serial.println("Sending: " + (String)send_message);
        sendLoRa();
      }
    }
    else {
      String display_string = "";
      Serial.print(c);
      display.print(c);
      display.display();
      last_cursor_x = display.getCursorX();
      last_cursor_y = display.getCursorY();
      display_string.concat(c);
      send_message.concat(display_string);
    }
  }
}

void displayLoRa(char* message) {
  // Clear the receive line
  display.setTextColor(WHITE, BLACK);
  display.setCursor(0, 0);
  for (int i = 0; i < SCREEN_WIDTH / LETTER_WIDTH; i++) {
    display.print(" ");
  }
  display.display();

  // Display the message on the receive line
  display.setCursor(0, 0);
  display.print(message);
  display.display();

  // Put the cursor back where it was
  display.setCursor(last_cursor_x, last_cursor_y);
}

void sendLoRa() {
  char data[send_message.length() + 1] = {};
  send_message.toCharArray(data, send_message.length() + 1);

  Serial.println("\n" + send_message);
  
  Serial.println(F("Sending message"));
  rf95.send((uint8_t*)data, sizeof(data));
  rf95.waitPacketSent();
  //Serial.println(F("Sent"));
  send_message = "";
}

void receiveLoRa() {
  if (rf95.available())
  {
    // Should be a message for us now
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);

    if (rf95.recv(buf, &len))
    {
      digitalWrite(LED, HIGH);
      RH_RF95::printBuffer("Received: ", buf, len);
      Serial.print("Got: ");
      Serial.println((char*)buf);
      displayLoRa(buf);
       Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);

      // Send a reply
      //uint8_t data[] = "And hello back to you";
      //rf95.send(data, sizeof(data));
      //rf95.waitPacketSent();
      //Serial.println("Sent a reply");
      digitalWrite(LED, LOW);
    }
    else
    {
      //Serial.println("Receive failed");
    }
  }
}

void loop() {
  currentTime = millis();
  
  handleKeyboardInput(keyboard_obj.main(currentTime));

  receiveLoRa();
}
