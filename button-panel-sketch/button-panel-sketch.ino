#include <Joystick.h>

// DEBUG
//#define DEBUG_SERIAL
//#define DEBUG_TIME
//#define DEBUG_SKIP_USB
//#define DEBUG_SKIP_BEFORE_ROTARYMODE1 // do not send joystick events until rotary1 is klicked once

// connection pins
#define SLIDER1 A0
#define SLIDER2 A2
#define KEYPAD1 A1
#define ROTARY1_CLK 3   // need to support interrupts!
#define ROTARY1_DT 2
#define ROTARY1_SW 4

// ROTARY1: Every two buttons will enable an additional mode by pressing rotary1 switch
const int rotarty1Buttons[] = {24, 25, 26, 27, 28, 29, 30, 31};       

// KEYPAD: The keypad supports exactly 24 buttons.
// 12 from this list and additional 12 in the 2nd mode starting with #12
const int keyPadButtonIds[] = {8, 4, 0, 9, 5, 1, 10, 6, 2, 11, 7, 3};

// start of internal settings - change only if you understand the consequences!
const int rotaryModeCount = (sizeof(rotarty1Buttons)/sizeof(*rotarty1Buttons)) >> 1;
const int keyPadResistorValues[] = {400, 500, 530, 560, 590, 630, 670, 720, 770, 830, 910, 1000, 1030};

#define DEBOUNCE_DELAY_MS 5
#define DELAY_TIME_MS 40

#define NO_BUTTON -1
#define NO_VALUE -1

#define DIRECTION_LEFT 0
#define DIRECTION_RIGHT 1
#define DIRECTION_NONE -1

#define SLIDER_MIN 0
#define SLIDER_MAX 500

Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID,JOYSTICK_TYPE_GAMEPAD,
  32, 0,                 // Button Count, Hat Switch Count
  false, false, false,   // No X and Y, Z Axis
  true, true, false,     // Rx, Ry, no Rz Axis
  false, false,          // No rudder or throttle
  false, false, false);  // No accelerator, brake, or steering

int currentKeyPad1Button = NO_BUTTON;
int keyPad1ButtonPress = NO_BUTTON;
int keyPad1ButtonRelease = NO_BUTTON;

int currentSlider1Value = NO_VALUE;
int previousSlider1Value = NO_VALUE;

volatile int rotary1Direction = DIRECTION_NONE;
int currentRotary1Mode = 0;
int previousRotary1Button = NO_BUTTON;

bool currentRotary1SwitchState = false;
bool previousRotary1SwitchState = false; 

void setup() {
  Joystick.begin(false);
  Joystick.setRxAxisRange(SLIDER_MIN, SLIDER_MAX);
  Joystick.setRyAxisRange(SLIDER_MIN, SLIDER_MAX);

#ifdef DEBUG_SKIP_BEFORE_ROTARYMODE1
  currentRotary1Mode = -1;
#endif

#ifdef DEBUG_SERIAL  
  Serial.begin(19200);
#else if DEBUG_TIME
  Serial.begin(19200);
#endif
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(ROTARY1_SW, INPUT_PULLUP);
  pinMode(ROTARY1_CLK, INPUT);
  pinMode(ROTARY1_DT, INPUT);
  attachInterrupt(digitalPinToInterrupt(ROTARY1_CLK), Rotary1Interrupt, LOW);
}

void Rotary1Interrupt() {
  static unsigned long lastRotary1InterruptTime = 0;
  unsigned long currentTime = millis();
  
  if(currentTime - lastRotary1InterruptTime < DEBOUNCE_DELAY_MS) {
    return;
  }
  rotary1Direction = digitalRead(ROTARY1_DT) == LOW ? DIRECTION_LEFT : DIRECTION_RIGHT;    
  lastRotary1InterruptTime = currentTime;
}

void loop() {
  unsigned long curretnExecutionTime = millis();

  ReadRotary1Switch();
  ReadKeyPad1();
  ReadSlider1();

#ifdef DEBUG_SERIAL 
  ReportSerial();
#endif  

#ifdef DEBUG_TIME  
  ReportTiming();
#endif

  ReportUsb();
  DeleayNextExecution(curretnExecutionTime);
}

inline void DeleayNextExecution(unsigned long lastExecution) {  
  digitalWrite(LED_BUILTIN, LOW);
  unsigned long nextExecution = lastExecution + DELAY_TIME_MS;
  while(millis() < nextExecution) {
    // prevent interrupts from shortcutting the delay
    delay(nextExecution - millis());
  }
  digitalWrite(LED_BUILTIN, HIGH);
}

void ReportSerial() { 
  PrintInputValue("Rotary1Mode", currentRotary1Mode + 1);

  if(keyPad1ButtonRelease != NO_BUTTON) {
    PrintInputValue("KeyPad1Release", keyPad1ButtonRelease);
  }

  if(keyPad1ButtonPress != NO_BUTTON) {
    PrintInputValue("KeyPad1Press", keyPad1ButtonPress);
  }

  if(currentSlider1Value != NO_VALUE) {
    PrintInputValue("Slider1", currentSlider1Value);
  }

  int currentRotary1Button = GetRotary1ButtonForCurrentMode();
  if(currentRotary1Button != NO_BUTTON) {
    PrintInputValue("Rotary1Press", currentRotary1Button);
  }  
}

void ReportUsb() {
  bool sendData = false;
  
  if(keyPad1ButtonRelease != NO_BUTTON)  {
    PrintInputValue("Rlease", keyPad1ButtonRelease);
    Joystick.setButton(keyPad1ButtonRelease, 0);
    keyPad1ButtonRelease = NO_BUTTON;
    currentKeyPad1Button = NO_BUTTON;
    sendData = true;
  }

  if(keyPad1ButtonPress != NO_BUTTON) {
    PrintInputValue("Press", keyPad1ButtonPress);
    Joystick.setButton(keyPad1ButtonPress, 1);
    currentKeyPad1Button = keyPad1ButtonPress;
    keyPad1ButtonPress = NO_BUTTON;
    sendData = true;
  }
  
  if(currentSlider1Value != NO_VALUE) {
    Joystick.setRxAxis(currentSlider1Value);
    currentSlider1Value = NO_VALUE;
    sendData = true;
  }

  int currentRotary1Button = GetRotary1ButtonForCurrentMode();
  if(previousRotary1Button != NO_BUTTON && previousRotary1Button != currentRotary1Button) {
    Joystick.setButton(previousRotary1Button, 0);
    previousRotary1Button = NO_BUTTON;
    sendData = true;
  }

  if(currentRotary1Button != NO_BUTTON) {
    Joystick.setButton(currentRotary1Button, 1);
    previousRotary1Button = currentRotary1Button;
    rotary1Direction = DIRECTION_NONE; 
    sendData = true;
  }

#ifdef DEBUG_SKIP_BEFORE_ROTARYMODE1
  if(currentRotary1Mode < 0) {
    sendData = false;
  }
#endif

#ifndef DEBUG_SKIP_USB
  if(sendData) {
    Joystick.sendState();    
  }
#endif
}

void ReadRotary1Switch() {
  currentRotary1SwitchState = !digitalRead(ROTARY1_SW);
  if(previousRotary1SwitchState != currentRotary1SwitchState) {    
    if(currentRotary1SwitchState) {
      IncreaseRotary1Mode();
    }
    previousRotary1SwitchState = currentRotary1SwitchState;
  }
}

void IncreaseRotary1Mode() {
  if(++currentRotary1Mode >= rotaryModeCount)
    currentRotary1Mode = 0;
}

int GetRotary1ButtonForCurrentMode() {
  if(rotary1Direction == DIRECTION_NONE)
    return NO_BUTTON;
      
  return rotarty1Buttons[currentRotary1Mode * 2 + rotary1Direction];
}

void ReadKeyPad1() {
  int newButton = ReadKeyPadButton(KEYPAD1);
  if(newButton != currentKeyPad1Button) {
    ReleaseCurrentKeyPad1Button();
    PressKeyPad1Button(newButton);
  }
}

inline int ReadKeyPadButton(int keyPadId) {  
  return convertAnalogToKeyPadButton(analogRead(keyPadId));
}

inline void ReleaseCurrentKeyPad1Button() {
  keyPad1ButtonRelease = currentKeyPad1Button;  
}

inline void PressKeyPad1Button(int newButton) {
  keyPad1ButtonPress = newButton;
}

void ReadSlider1() {
  int newSliderValue = ReadSliderValue(SLIDER1);
  if(newSliderValue != previousSlider1Value) {
    previousSlider1Value = newSliderValue;
    currentSlider1Value = newSliderValue;
  }
}

int ReadSliderValue(int sliderId) {
  return convertAnalogToSliderValue(analogRead(sliderId));
}

void ReportTiming() {
  static unsigned long lastReportTime = 0;
  unsigned long currentTime = millis();
  
  if(currentTime > lastReportTime) {
    Serial.print(currentTime - lastReportTime);
    Serial.println(" ms");
  }
  lastReportTime = currentTime;
}

inline int convertAnalogToKeyPadButton(int analogValue) {
  if(analogValue <= 100)
    return NO_BUTTON;
    
  int buttonIndex=0;
  do {
  } while(analogValue > keyPadResistorValues[buttonIndex++]);  
  
  return keyPadButtonIds[buttonIndex - 2];
}

inline int convertAnalogToSliderValue(int analogValue) {
  return map(analogValue, 0, 1023, SLIDER_MIN, SLIDER_MAX);  
}

inline void PrintInputValue(const char* label, int value) {   
  Serial.print(label);
  Serial.print(": ");
  Serial.println(value);
}
