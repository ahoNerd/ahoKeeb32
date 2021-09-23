#define devMode true // uncomment untuk mengaktifkan Serial
// #define kusoRGB true // uncomment kalo pake rgbLed
#define kusoRotary true // uncomment kalo pake rotary encoder
#define kusoOled true // uncomment kalo pake oled

#include <BleKeyboard.h>
#include "numpad.h"

#define oledDa         32
#define oledCk         33
#define ledPin         25
#define rotaryDT       27
#define rotaryCL       26
#define mutePin        14

#define bleName        "ahoKeeb32"
#define bleMfg         "ahoNerd"
#define bounce         27
#define oledTimeout    15000
#define ledCount       2

#ifdef kusoOled
#include "SSD1306.h" // alias for `#include "SSD1306Wire.h"` // lib ver 4.2.1
#include "font.h" //http://oleddisplay.squix.ch/
#include "logo.h"
SSD1306  display(0x3c, oledDa, oledCk); // SDA, SCK
#ifndef DISPLAY_WIDTH
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#endif
#define TENGAH_H DISPLAY_WIDTH / 2
#endif

BleKeyboard bleKeyboard(bleName);

#ifdef kusoRGB
#include <esp32WS2811.h>
#include <Ticker.h>
WS2811 ws2811(ledPin, ledCount); // first argument is the data pin, the second argument is the number of LEDs
Ticker timer;
#endif

const byte qtyRoteryLayer = 5;
const byte qtyLayer = 4;
const byte rows = 5;
const byte cols = 3;
const byte maxCombi = 3; // key combination
const byte rowPins[rows] = {4, 0, 2, 15, 13};
const byte colPins[cols] = {5, 17, 16};
// const byte* btnMedia[rows][cols]  = {
//   {   KEY_MEDIA_VOLUME_DOWN,  KEY_MEDIA_VOLUME_UP},
//   {KEY_MEDIA_PREVIOUS_TRACK, KEY_MEDIA_NEXT_TRACK}
// };
const byte keyMap[qtyLayer][rows][cols][maxCombi] = {
  {
    {                     {KEY_ESC},                        {KEY_BACKSPACE},                    {KEY_DELETE}},
    {                    {KEY_HOME},                         {KEY_UP_ARROW},                       {KEY_END}},
    {              {KEY_LEFT_ARROW},                       {KEY_DOWN_ARROW},               {KEY_RIGHT_ARROW}},
    {{KEY_LEFT_ALT, KEY_LEFT_ARROW},   {KEY_LEFT_CTRL, KEY_LEFT_SHIFT, 'i'}, {KEY_LEFT_ALT, KEY_RIGHT_ARROW}},
    {                         {' '},              {KEY_LEFT_SHIFT, KEY_TAB},                       {KEY_TAB}}
  },
  {
    {                     {KEY_ESC},                              {KC_PAST},                       {KC_PPLS}},
    {                         {'7'},                                  {'8'},                           {'9'}},
    {                         {'4'},                                  {'5'},                           {'6'}},
    {                         {'1'},                                  {'2'},                           {'3'}},
    {                  {KEY_RETURN},                                  {'0'},                           {'.'}}
  },
  {
    {                     {KEY_ESC},                   {KEY_LEFT_CTRL, 's'},          {KEY_LEFT_ALT, KEY_F4}},
    {          {KEY_LEFT_CTRL, 'f'},   {KEY_LEFT_CTRL, KEY_LEFT_SHIFT, 'f'},            {KEY_LEFT_CTRL, 'h'}},
    {          {KEY_LEFT_CTRL, 'z'},   {KEY_LEFT_CTRL, KEY_LEFT_SHIFT, 'z'},            {KEY_LEFT_CTRL, 'y'}},
    {          {KEY_LEFT_CTRL, 'x'},                   {KEY_LEFT_CTRL, 'c'},            {KEY_LEFT_CTRL, 'v'}},
    {                  {KEY_RETURN}, {KEY_LEFT_CTRL, KEY_LEFT_ALT, KEY_TAB},        {KEY_LEFT_CTRL, KEY_TAB}}
  },
  {
    {                     {KEY_F10},                              {KEY_F11},                       {KEY_F12}},
    {                      {KEY_F7},                               {KEY_F8},                        {KEY_F9}},
    {                      {KEY_F4},                               {KEY_F5},                        {KEY_F6}},
    {                      {KEY_F1},                               {KEY_F2},                        {KEY_F3}},
    {                  {KEY_RETURN},                        {KEY_LEFT_CTRL},                  {KEY_LEFT_ALT}}
  }
};
const byte keyAltMap[qtyLayer][rows][cols][maxCombi] = { // jangan isi shiftLayer & shiftRotaryLayer
  {0},
  {
    {                           {0},                              {KC_PSLS},                       {KC_PMNS}},
    {0},
    {0},
    {0},
    {                           {0},                                    {0},                 {KEY_BACKSPACE}}
  },
  {0},
  {0}
};
#ifdef kusoOled
const String buttonDsc[qtyLayer][rows][cols] = {
  {
    {    "Esc",  "backspc",        "Del"},
    {   "home",       "up",        "end"},
    {   "left",     "down",      "right"},
    {   "back",      "Dev",        "fwd"},
    {  "space",    "unTab",        "Tab"}
  },
  { 
    {    "Esc",    "x / ÷",      "+ / -"},
    {      "7",        "8",          "9"},
    {      "4",        "5",          "6"},
    {      "1",        "2",          "3"},
    {  "Enter",        "0",". / backspc"}
  },
  {
    {    "Esc",     "save",      "close"},
    {   "find", "find all",    "replace"},
    {   "undo",     "redo",       "redo"},
    {    "cut",     "copy",      "paste"},
    {  "Enter",   "sw Win",     "sw Tab"}
  },
  {
    {    "F10",      "F11",        "F12"},
    {     "F7",       "F8",         "F9"},
    {     "F4",       "F5",         "F6"},
    {     "F1",       "F2",         "F3"},
    {  "Enter",     "ctrl",        "alt"}
  }
};
const String layerName[qtyLayer] = {"Navigation", "Numpad", "Editing", "Function"};
const String rotaryLayerName[qtyRoteryLayer] = {"volume", "photoShop", "Sublime", "Undo", "Undo (Y)"};
#endif
bool buttonStates[rows][cols] = {false};
long lastPressed[rows][cols]  = {0};
bool keyPressed[rows][cols]   = {false};
bool bleConnected             = false;
long lastDisp                 = 0;
byte layer                    = 0;
byte rotaryLayer              = 0;
long lastMute                 = 0;
bool oledOn                   = true;
bool mutePinState             = false;
bool fn1                      = false;
byte Fn1State                 = 0;

#ifdef kusoRotary
uint8_t lrmem = 3;
int lrsum = 0;
void myRotary() {
  int8_t res;
  res = rotary();
  if (res!=0) {
    if (res == 1) {
      if (oledOn) {
        switch (rotaryLayer) {
          case 0:
            bleKeyboard.write(KEY_MEDIA_VOLUME_UP);
            break;
          case 1:
            bleKeyboard.write(']');
            break;
          case 2:
            bleKeyboard.press(KEY_LEFT_CTRL);
            bleKeyboard.write(']');
            sendRelease();
            break;
          case 3:
            bleKeyboard.press(KEY_LEFT_CTRL);
            bleKeyboard.press(KEY_LEFT_SHIFT);
            bleKeyboard.write('z');
            sendRelease();
            break;
          case 4:
            bleKeyboard.press(KEY_LEFT_CTRL);
            bleKeyboard.write('y');
            sendRelease();
            break;
        }
      }
    }
    if (res == -1) {
      if (oledOn) {
        switch (rotaryLayer) {
          case 0:
            bleKeyboard.write(KEY_MEDIA_VOLUME_DOWN);
            break;
          case 1:
            bleKeyboard.write('[');
            break;
          case 2:
            bleKeyboard.press(KEY_LEFT_CTRL);
            bleKeyboard.write('[');
            sendRelease();
            break;
          case 3:
            bleKeyboard.press(KEY_LEFT_CTRL);
            bleKeyboard.write('z');
            sendRelease();
            break;
          case 4:
            bleKeyboard.press(KEY_LEFT_CTRL);
            bleKeyboard.write('z');
            sendRelease();
            break;
        }
      }
    }
    turnOnOled();
  }
}

int8_t rotary() { // https://www.pinteric.com/rotary.html?fbclid=IwAR2RjxjL8GYMxHs3CC-NjQ7h28q_e-8uD30w1AajcEHVSPI7ZAuHwlF1CTI
  static int8_t TRANS[] = {0,-1,1,14,1,0,14,-1,-1,14,0,1,14,1,-1,0};
  int8_t l, r;
  const byte cycle = 2; // normalnya 4 (tapi jadi perlu 2 step untuk 1)
  l = digitalRead(rotaryCL);
  r = digitalRead(rotaryDT);
  lrmem = ((lrmem & 0x03) << 2) + 2*l + r;
  lrsum = lrsum + TRANS[lrmem];
  if (lrsum % cycle != 0) return(0); // encoder not in the neutral state
  if (lrsum == cycle) { // encoder in the neutral state
    lrsum=0;
    return(1);
  }
  if (lrsum == -cycle) {
    lrsum = 0;
    return(-1);
  }
  lrsum = 0; // lrsum > 0 if the impossible transition
  return(0);
}

void shiftRotaryLayer() {
  rotaryLayer++;
  if (rotaryLayer >= qtyRoteryLayer) {
    rotaryLayer = 0;
  }
}

void rotarySw(long now) {
  if (now - lastMute > bounce) {
    lastMute = millis();
    bool state = digitalRead(mutePin); // untuk mencegah repetisi
    if (state != mutePinState) {
      if (state == LOW) {
        bleKeyboard.write(KEY_MEDIA_MUTE);
      }
      mutePinState = state;
    }
  }
}
#endif

#ifdef kusoOled
void setup_display() {
  oledOn = false; // false supaya kalo lagi loading tetap nyala
  display.init();
  // display.invertDisplay();
  display.flipScreenVertically();
  display.setColor(INVERSE);
  display.drawXbm(32, 0, ahonerd_width, ahonerd_height, ahonerd_bits);
  // display.setFont(ArialMT_Plain_16);
  // display.setTextAlignment(TEXT_ALIGN_CENTER);
  // display.drawString(DISPLAY_WIDTH / 2, 24, "Connecting");
  display.display();
}

void turnOnOled() {
  lastDisp = millis(); // reset oled timeout
  if (!oledOn) {
    display.displayOn();
    oledOn = true;
    #ifdef devMode
    Serial.println("Turning oled ON");
    #endif
  }
}

void oledLoop(long now) {
  if (oledOn) {
    if (now - lastDisp > oledTimeout) {
      // lastDisp = millis(); // kalo ga ada masalah hapus aja
      display.displayOff();
      oledOn = false;
      #ifdef devMode
      Serial.println("Turning oled OFF");
      #endif
    }
  }
}

#define TENGAH_1 TENGAH_H - (net_up_03_width / 2)
const byte oledY[rows] = {12, 22, 32, 42, 52};
void refDisp() {
  turnOnOled();
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  if (bleConnected) {
    if (fn1 || Fn1State) {
      if (Fn1State == 0) {
        display.drawXbm(TENGAH_1, 2, net_up_03_width, net_up_03_height, net_up_03_bits);
      } else if (Fn1State == 1) {
        display.drawXbm(TENGAH_1, 0, net_down_03_width, net_down_03_height, net_down_03_bits);
      } else {
        display.drawXbm(TENGAH_1, 2, net_down_03_width, net_down_03_height, net_down_03_bits);
      }
      #ifdef devMode
      display.drawString(TENGAH_H + net_up_03_width, 0, String(Fn1State));
      #endif
    }
    for(byte i = 0; i < rows; i++) {
      display.drawString(TENGAH_H, oledY[i], buttonDsc[layer][i][1]);
    }
    byte dscWidth, dscStart; 
    for(byte col = 0; col < cols; col++) {
      for (byte row = 0; row < rows; row++) {
        if (keyPressed[row][col]) {
          dscWidth = display.getStringWidth(buttonDsc[layer][row][col]) + 2;
          switch (col) {
            case 0:
              dscStart = 0;
              break;
            case 1:
              dscStart = TENGAH_H - (dscWidth / 2);
              break;
            case 2:
              dscStart = DISPLAY_WIDTH - dscWidth;
              break;
          }
          display.fillRect(dscStart, oledY[row] + 2, dscWidth, 10); // 10 dari font size
        }
      }
    }
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    for(byte i = 0; i < rows; i++) {
      display.drawString(1, oledY[i], buttonDsc[layer][i][0]);
    }
    display.drawString(0, 0, layerName[layer]);
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    for(byte i = 0; i < rows; i++) {
      display.drawString(DISPLAY_WIDTH - 1, oledY[i], buttonDsc[layer][i][2]);
    }
    display.drawLine(0, 12, DISPLAY_WIDTH, 12);
    display.drawString(DISPLAY_WIDTH, 0, rotaryLayerName[rotaryLayer]);
  } else {
    display.setFont(ArialMT_Plain_16);
    display.drawString(TENGAH_H, 24, "Disconnected");
  }
  display.display();
}
#endif

void shiftLayer() {
  layer++;
  if (layer >= qtyLayer) {
    layer = 0;
  }
}

void sendKey(const byte theKey[maxCombi], bool isPressed) {
  for(byte i = 0; i < maxCombi; i++) {
    if (theKey[i] != 0) { // karena ukuran array = maxCombi tapi tidak semua element berisi segitu, malah ada yg cuma 1
      if (isPressed) {
        bleKeyboard.press(theKey[i]);
        #ifdef devMode
        Serial.print("Pressed: ");
        Serial.println(theKey[i]);
        #endif
      } else {
        bleKeyboard.release(theKey[i]);
        #ifdef devMode
        Serial.print("Released: ");
        Serial.println(theKey[i]);
        #endif
      }
    }
  }
}

long lastRelease = 0;
bool reqRelease = false; // request release
void sendRelease() {
  lastRelease = millis();
  reqRelease = true;
}

void releaseKey(long now) {
  if (reqRelease) {
    if (now - lastRelease > bounce) {
      reqRelease = false;
      bleKeyboard.releaseAll();
      #ifdef devMode
      Serial.println("Released All");
      #endif
    }
  }
}

void osu(bool state, byte r, byte c) {
  if (state == LOW) { // pressed
    keyPressed[r][c] = true;
    if (fn1) {
      Fn1State = 0;
      for(byte col = 0; col < cols; col++) {
        for (byte row = 0; row < rows; row++) {
          if (keyPressed[row][col]) {
            Fn1State++;
          }
        }
      }
      if (r == 4 && c == 1) {
        shiftLayer();
        #ifdef devMode
        Serial.println("shiftLayer");
        #endif
      } else
      #ifdef kusoRotary
      if (r == 3 && c == 0) {
        shiftRotaryLayer();
        #ifdef devMode
        Serial.println("shiftRotaryLayer");
        #endif
      } else
      #endif
      {
        sendKey(keyAltMap[layer][r][c], true);
      }
    } else if (r == 4 && c == 0) {
      fn1 = true;
    } else {
      sendKey(keyMap[layer][r][c], true);
    }
  } else { // released
    keyPressed[r][c] = false;
    if (Fn1State) {
      if (fn1) {
        sendKey(keyAltMap[layer][r][c], false);
      }
      if (r == 4 && c == 0) {
        fn1 = false;
        if (Fn1State > 0) {
          bleKeyboard.releaseAll();
          #ifdef devMode
          Serial.println("Released All");
          #endif
        }
      }
      Fn1State--;
    } else if (r == 4 && c == 0) {
      fn1 = false;
      sendKey(keyMap[layer][r][c], true);
      sendRelease();
    } else {
      sendKey(keyMap[layer][r][c], false);
    }
  }
}

#ifdef kusoRGB
byte rVal = 10;
byte gVal = 1;
byte bVal = 20;
byte rDir = -1;
byte gDir = 1;
byte bDir = -1;
void nextEffect() {
  for (byte i = 0; i < ledCount; i++) {
    rVal = rVal + rDir; // changing values of LEDs
    gVal = gVal + gDir;
    bVal = bVal + bDir;
    if (rVal >= 21 || rVal <= 0) {
      rDir = rDir * -1;
    }
    if (gVal >= 21 || gVal <= 0) {
      gDir = rDir * -1;
    }
    if (bVal >= 21 || bVal <= 0) {
      bDir = bDir * -1;
    }
    ws2811.setPixel(i,   rVal,   gVal,   bVal);
  }
  ws2811.show();
}
#endif

void setup() {
  for (byte r = 0; r < rows; r++) {
    pinMode(rowPins[r], INPUT_PULLUP);
  }
  for (byte c = 0; c < cols; c++) {
    pinMode(colPins[c], OUTPUT);
  }
  #ifdef kusoRotary
  pinMode(rotaryDT, INPUT_PULLUP);
  pinMode(rotaryCL, INPUT_PULLUP);
  pinMode(mutePin, INPUT_PULLUP);
  #endif
  #ifdef kusoOled
  setup_display();
  #endif
  #ifdef devMode
  Serial.begin(115200);
  Serial.println("Starting BLE work!");
  #endif
  bleKeyboard.begin();
  #ifdef kusoRGB
  ws2811.begin();
  ws2811.setAll(  0,   0,   0);  // gives all LEDs the specified color
  ws2811.show();
  // ws2811.clearAll();  // turns off all LEDs
  timer.attach(0.1, nextEffect);
  #endif
}

void loop() {
  long now = millis();
  #ifdef kusoOled
  oledLoop(now);
  #endif
  #ifdef kusoRotary
  myRotary();
  rotarySw(now);
  #endif
  releaseKey(now);
  bool bState = bleKeyboard.isConnected();
  if (bState) {
    for(byte c = 0; c < cols; c++) {
      digitalWrite(colPins[c], LOW);
      for (byte r = 0; r < rows; r++) {
        if (now - lastPressed[r][c] > bounce) {
          lastPressed[r][c] = millis();
          bool state = digitalRead(rowPins[r]); // ga perlu repetisi cuz yg dikirim press then release
          if (state != buttonStates[r][c]) {
            osu(state, r, c);
            #ifdef kusoOled
            refDisp();
            #endif
            buttonStates[r][c] = state;
          }
        }
      }
      digitalWrite(colPins[c], HIGH);
    }
  }
  if (bState != bleConnected) {
    Fn1State = 0;
    bleConnected = bState;
    #ifdef kusoOled
    refDisp();
    #endif
    #ifdef devMode
    Serial.println(bleConnected ? "Connected" : "Disconnected");
    #endif
  }
}
