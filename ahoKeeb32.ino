#include <BleKeyboard.h>

#define bleName     "kuso BLE Keyboard"
#define bounce      27

BleKeyboard bleKeyboard(bleName);

byte buttonPins[]      = {          22,             21,             19,              18};
char buttonChars[]     = {KEY_UP_ARROW, KEY_DOWN_ARROW, KEY_LEFT_ARROW, KEY_RIGHT_ARROW};
boolean buttonStates[] = {       false,          false,          false,           false};
long lastPressed[]     = {           0,              0,              0,               0};

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE work!");
  bleKeyboard.begin();
  for (int i = 0; i < sizeof(buttonPins); i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }
}

void loop() {
  long now=millis();
  if (bleKeyboard.isConnected()) {
    for (int i = 0; i < sizeof(buttonPins); i++) {
      if (now - lastPressed[i] > bounce) {
        lastPressed[i] = millis();
        boolean state = digitalRead(buttonPins[i]);
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
}
