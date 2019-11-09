#include <Joystick.h>

// DEBUG
#define DEBUG_SERIAL
//#define DEBUG_TIME
//#define DEBUG_SKIP_USB

// connection pins
#define SLIDER1 A0
#define SLIDER2 A2
#define KEYPAD1 A1
#define ROTARY1_CLK 3   // has to support interrupt!
#define ROTARY1_DT 2
#define ROTARY1_SW 4

const int rotarty1Buttons[] = {13, 14, 15, 16, 17, 18, 19, 20};        // add two buttons for each rotary1 mode
const int keyPadButtonIds[] = {8, 4, 0, 9, 5, 1, 10, 6, 2, 11, 7, 3};  // keypad supports exactly 12 buttons
const int rotaryModeCount = (sizeof(rotarty1Buttons)/sizeof(*rotarty1Buttons)) >> 1;

#define DEBOUNCE_DELAY_MS 5
#define DELAY_TIME_MS 200     // use less then 50 ms for non testing environment

// start of internals - only change if you understand the consequences!
#define NO_BUTTON -1
#define NO_VALUE -1

#define DIRECTION_LEFT 0
#define DIRECTION_RIGHT 1
#define DIRECTION_NONE -1

Joystick_ Joystick;

const int maxRotaryButtonIndex = 7;
const int keyPadResistorValues[] = {400, 500, 530, 560, 590, 630, 670, 720, 770, 830, 910, 1000, 1030};

int currentKeyPad1ButtonPress = NO_BUTTON;
int currentKeyPad1ButtonRelease = NO_BUTTON;

int currentSlider1Value = NO_VALUE;
int previousSlider1Value = NO_VALUE;

volatile int rotary1Direction = DIRECTION_NONE;
int currentRotary1Mode = -1;
int previousRotary1Button = NO_BUTTON;

bool currentRotary1SwitchState = false;
bool previousRotary1SwitchState = false; 

void setup() {
#ifdef DEBUG_SERIAL  
  Serial.begin(115200);
#else if DEBUG_TIME
  Serial.begin(115200);
#endif
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(ROTARY1_SW, INPUT_PULLUP);
  pinMode(ROTARY1_CLK, INPUT);
  pinMode(ROTARY1_DT, INPUT);
  attachInterrupt(digitalPinToInterrupt(ROTARY1_CLK), Rotary1Interrupt, LOW);
  Joystick.begin(false);
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
  ReadRotary1Switch();
  ReadKeyPad1();
  ReadSlider1();

#ifdef DEBUG_SERIAL 
  ReportSerial();
#endif  

#ifdef DEBUG_TIME  
  PrintTimeReport();
#endif

  ReportUsb();
  delay(DELAY_TIME_MS);
}

void ReportSerial() { 
  printSensorValue("Rotary1 Mode", currentRotary1Mode + 1);

  if(rotary1Direction != DIRECTION_NONE)
    printSensorValue("Rotary1 Direction", rotary1Direction);

  if(currentKeyPad1ButtonPress != NO_BUTTON)
    printSensorValue("KeyPad1 Button", currentKeyPad1ButtonPress);

  if(currentKeyPad1ButtonRelease != NO_BUTTON)
    printSensorValue("KeyPad1 Release", currentKeyPad1ButtonRelease);

  if(currentSlider1Value != NO_VALUE)
    printSensorValue("Slider1", currentSlider1Value);
}

void ReportUsb() {
  if(currentKeyPad1ButtonPress != NO_BUTTON)
    Joystick.setButton(currentKeyPad1ButtonPress, 1);

  if(currentKeyPad1ButtonRelease != NO_BUTTON) {
    Joystick.setButton(currentKeyPad1ButtonRelease, 0);
    currentKeyPad1ButtonRelease = NO_BUTTON;
  }

  if(currentSlider1Value != NO_VALUE) {
    ; // TODO
    currentSlider1Value = NO_VALUE;
  }

  int currentRotary1Button = GetRotary1ButtonForCurrentMode();
  if(previousRotary1Button != NO_BUTTON && previousRotary1Button != currentRotary1Button) {
    Joystick.setButton(previousRotary1Button, 0);
    previousRotary1Button = NO_BUTTON;
  }

  if(currentRotary1Button != NO_BUTTON) {
    Joystick.setButton(currentRotary1Button, 1);
    previousRotary1Button = currentRotary1Button;
    rotary1Direction = DIRECTION_NONE; 
  }

#ifndef DEBUG_SKIP_USB  
  digitalWrite(LED_BUILTIN, HIGH);
  // do not send joystick events before clicking a rotary button at least once
  if(currentRotary1Mode >= 0) {
    Joystick.sendState();
  }
  digitalWrite(LED_BUILTIN, LOW);  
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
  if(newButton != currentKeyPad1ButtonPress) {
    ReleaseCurrentKeyPad1Button();
  }
  PressKeyPad1Button(newButton);
}

inline int ReadKeyPadButton(int keyPadId) {
  return convertAnalogToKeyPadButton(analogRead(keyPadId));
}

inline void ReleaseCurrentKeyPad1Button() {
  currentKeyPad1ButtonRelease = currentKeyPad1ButtonPress;  
}

inline void PressKeyPad1Button(int newButton) {
  currentKeyPad1ButtonPress = newButton;  
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

void PrintTimeReport() {
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
  return map(analogValue, 0, 1023, 0, 100);  
}

inline void printSensorValue(const char* label, int value) {   
  Serial.print(label);
  Serial.print(": ");
  Serial.println(value);
}
