#include <Joystick.h>

// DEBUG
//#define DEBUG_SERIAL
//#define DEBUG_TIME
//#define DEBUG_SKIP_JOYSTICK_UPDATE
//#define DEBUG_SKIP_BEFORE_ROTARYMODE1 // do not send joystick events until rotary1 is clicked once

// connection pins
#define SLIDER1 A0
#define SLIDER2 A2
#define KEYPAD1 A1
#define ROTARY1_CLK 3   // need to support interrupts!
#define ROTARY1_DT 2
#define ROTARY1_SW 4

// ROTARY1: Every two buttons will enable an additional mode by pressing rotary1 switch
const int rotarty1Buttons[] = {24, 25, 26, 27, 28, 29, 30, 31};       

// KEYPAD: The keypad supports exactly 24 buttons
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

// Stop slider axis flattering between two values by downscaling resolution (max=0:1023)
#define SLIDER_MIN 0
#define SLIDER_MAX 341

Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_JOYSTICK,
  32, 0,                 // Button Count, Hat Switch Count
  false, false, false,   // No X and Y, Z Axis
  true, true, false,     // Rx, Ry, no Rz Axis
  false, false,          // No rudder or throttle
  false, false, false);  // No accelerator, brake, or steering

int currentKeyPad1Button = NO_BUTTON;
int currentSlider1Value = NO_VALUE;

volatile int rotary1Direction = DIRECTION_NONE;
volatile bool updateJoystick = true;

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
  EnableInterrupts();
}

void loop() {
  DisableInterrupts();
  
  unsigned long currentExecutionTime = millis();
  UpdateRotary1Mode();
  ReadRotary1();
  ReadKeyPad1();
  ReadSlider1();
  
  EnableInterrupts();
  
#ifdef DEBUG_TIME  
  ReportTiming();
#endif

#ifndef DEBUG_SKIP_JOYSTICK_UPDATE
  UpdateJoystick();
#endif

  DeleayNextExecution(currentExecutionTime);
}

inline void DisableInterrupts() {
  detachInterrupt(digitalPinToInterrupt(ROTARY1_CLK));
}

inline void EnableInterrupts() {
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

inline void DeleayNextExecution(unsigned long lastExecution) {  
  digitalWrite(LED_BUILTIN, LOW);
  unsigned long nextExecution = lastExecution + DELAY_TIME_MS;
  while(millis() < nextExecution) {
    // prevent interrupts from shortcutting the delay
    delay(nextExecution - millis());
  }
  digitalWrite(LED_BUILTIN, HIGH);
}

void UpdateJoystick() {
  if(updateJoystick) {
    updateJoystick = false;  

#ifdef DEBUG_SERIAL
  Serial.println("Sending state...");
#endif

#ifdef DEBUG_SKIP_BEFORE_ROTARYMODE1
    if(currentRotary1Mode >= 0) {
      Joystick.sendState();
    }
#else
    Joystick.sendState();
#endif
  }
}

void UpdateRotary1Mode() {
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
#ifdef DEBUG_SERIAL  
  PrintInputValue("Rotary1Mode", currentRotary1Mode + 1);
#endif  
}

int GetRotary1ButtonForCurrentMode() {
  if(rotary1Direction == DIRECTION_NONE)
    return NO_BUTTON;      
  return rotarty1Buttons[currentRotary1Mode * 2 + rotary1Direction];
}

void ReadRotary1() {
  int newButton = GetRotary1ButtonForCurrentMode();
  if(previousRotary1Button != NO_BUTTON && previousRotary1Button != newButton) {
    Joystick.setButton(previousRotary1Button, LOW);
    updateJoystick = true;
    previousRotary1Button = NO_BUTTON;
  }
  if(newButton != NO_BUTTON) {
    Joystick.setButton(newButton, HIGH);
    updateJoystick = true;
    previousRotary1Button = newButton;
    rotary1Direction = DIRECTION_NONE;
#ifdef DEBUG_SERIAL 
    PrintInputValue("Rotary1Button", newButton);
#endif       
  }  
}

void ReadKeyPad1() {
  int newButton = ReadKeyPadButton(KEYPAD1);
  if(newButton != currentKeyPad1Button) {
    ReleaseCurrentKeyPad1Button(currentKeyPad1Button);
    PressKeyPad1Button(newButton);    
  }
}

inline int ReadKeyPadButton(int keyPadId) {  
  return ConvertAnalogToKeyPadButton(analogRead(keyPadId));
}

inline void ReleaseCurrentKeyPad1Button(int button) {
  if(button != NO_BUTTON) {
    Joystick.setButton(button, LOW);
    updateJoystick = true;
#ifdef DEBUG_SERIAL 
    PrintInputValue("KeyPad1Release", button);
#endif      
  }
  currentKeyPad1Button = NO_BUTTON;
}

inline void PressKeyPad1Button(int button) {
  if(button != NO_BUTTON) {  
    Joystick.setButton(button, HIGH);
    updateJoystick = true;    
#ifdef DEBUG_SERIAL 
    PrintInputValue("KeyPad1Press", button);
#endif  
  }
  currentKeyPad1Button = button;
}

void ReadSlider1() {
  int newSliderValue = ReadSliderValue(SLIDER1);
  if(newSliderValue != currentSlider1Value) {
    Joystick.setRxAxis(newSliderValue);
    updateJoystick = true;
    currentSlider1Value = newSliderValue;
#ifdef DEBUG_SERIAL 
    PrintInputValue("Slider1", newSliderValue);
#endif  
  }
}

int ReadSliderValue(int sliderId) {
  return ConvertAnalogToSliderValue(analogRead(sliderId));
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

inline int ConvertAnalogToKeyPadButton(int analogValue) {
  if(analogValue <= 100)
    return NO_BUTTON;
    
  int buttonIndex=0;
  do {
  } while(analogValue > keyPadResistorValues[buttonIndex++]);  
  
  return keyPadButtonIds[buttonIndex - 2];
}

inline int ConvertAnalogToSliderValue(int analogValue) {
  return map(analogValue, 0, 1023, SLIDER_MIN, SLIDER_MAX);  
}

void PrintInputValue(const char* label, int value) {   
  Serial.print(label);
  Serial.print(": ");
  Serial.println(value);
}
