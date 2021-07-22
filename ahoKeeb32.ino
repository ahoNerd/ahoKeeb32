#define devMode true // uncoment untuk mengaktifkan Serial

#include <BleKeyboard.h>

#define bleName        "kuso BLE Keyboard"
#define bounce         27
#define oledDa         32
#define oledCk         33

#include "SSD1306.h" // alias for `#include "SSD1306Wire.h"`
#include "font.h" //http://oleddisplay.squix.ch/
SSD1306  display(0x3c, oledDa, oledCk); // SDA, SCK
#ifndef DISPLAY_WIDTH
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#endif

BleKeyboard bleKeyboard(bleName);

byte buttonPins[]      = {          16,             17,             19,              18};
char buttonChars[]     = {KEY_UP_ARROW, KEY_DOWN_ARROW, KEY_LEFT_ARROW, KEY_RIGHT_ARROW};
bool buttonStates[]    = {       false,          false,          false,           false};
long lastPressed[]     = {           0,              0,              0,               0};
bool bleConnected      = false;
long lastDisp          = 0;
bool refDisp           = false;

void setup_display() {
  display.init();
  // display.invertDisplay();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(DISPLAY_WIDTH/2, 24, "Loading");
  display.display();
}

void oledLoop(long now){
  if(refDisp){
  // if(refDisp || now - lastDisp > 999){
  //   lastDisp=millis();
    refDisp = false;
    display.clear();
    display.drawString(DISPLAY_WIDTH/2, 24, bleConnected ? "Connected" : "Disconnected");
    display.display();
    #ifdef devMode
    Serial.print("oled: ");
    Serial.print(millis());
    Serial.println(bleConnected ? " Connected" : " Disconnected");
    #endif
  }
}

void setup() {
  for (int i = 0; i < sizeof(buttonPins); i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }
  setup_display();
  #ifdef devMode
  Serial.begin(115200);
  Serial.println("Starting BLE work!");
  #endif
  bleKeyboard.begin();
}

void loop() {
  long now=millis();
  oledLoop(now);
  bool bState = bleKeyboard.isConnected();
  if (bState) {
    for (int i = 0; i < sizeof(buttonPins); i++) {
      if (now - lastPressed[i] > bounce) {
        lastPressed[i] = millis();
        bool state = digitalRead(buttonPins[i]);
        if (state != buttonStates[i]) {
          if (state == LOW) {
            bleKeyboard.press(buttonChars[i]);
          } else {
            bleKeyboard.release(buttonChars[i]);
          }
          buttonStates[i] = state;
        }
      }
    }
  }
  if (bState != bleConnected) {
    bleConnected = bState;
    refDisp = true;
  }
}
