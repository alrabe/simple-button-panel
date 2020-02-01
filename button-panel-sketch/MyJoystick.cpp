#include <Joystick.h>
#include "MySettings.h"
#include "MyJoystick.h"

// ROTARY1: Every two buttons will enable an additional mode by pressing rotary1 switch
const int rotarty1Buttons[] = {24, 25, 26, 27, 28, 29, 30, 31};       

// KEYPAD: The keypad supports exactly 24 buttons
// 12 from this list and additional 12 in the 2nd mode starting with #12
const int keyPadButtonIds[] = {8, 4, 0, 9, 5, 1, 10, 6, 2, 11, 7, 3};

// start of internal settings - change only if you understand the consequences!
const int rotaryModeCount = (sizeof(rotarty1Buttons)/sizeof(*rotarty1Buttons)) >> 1;
const int keyPadResistorValues[] = {400, 500, 530, 560, 590, 630, 670, 720, 770, 830, 910, 1000, 1030};

int currentKeyPad1Button = NO_BUTTON;
int currentSliderValue[] = {NO_VALUE, NO_VALUE};

volatile int rotary1Direction = DIRECTION_NONE;
volatile bool updateJoystick = true;

int currentRotary1Mode = 0;
int previousRotary1Button = NO_BUTTON;

bool currentRotary1SwitchState = false;
bool previousRotary1SwitchState = false;

Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_JOYSTICK,
    32, 0,                 // Button Count, Hat Switch Count
    false, false, false,   // No X and Y, Z Axis
    true, true, false,     // Rx, Ry, no Rz Axis
    false, false,          // No rudder or throttle
    false, false, false);  // No accelerator, brake, or steering

MyJoystick::MyJoystick() {
  Joystick.begin(false);
  Joystick.setRxAxisRange(SLIDER_MIN, SLIDER_MAX);
  Joystick.setRyAxisRange(SLIDER_MIN, SLIDER_MAX);

#ifdef DEBUG_SKIP_BEFORE_ROTARYMODE1
  currentRotary1Mode = -1;
#endif
}

void MyJoystick::Update() {
  UpdateRotary1Mode();
  ReadRotary1();
  ReadKeyPad1();
  ReadSlider1();
  ReadSlider2();
}

void MyJoystick::Report() {
#ifndef DEBUG_SKIP_JOYSTICK_UPDATE
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
#endif
  }
}

void MyJoystick::EnableInterrupts() {
  attachInterrupt(digitalPinToInterrupt(ROTARY1_CLK), Rotary1Interrupt, LOW);
}

void MyJoystick::DisableInterrupts() {
  detachInterrupt(digitalPinToInterrupt(ROTARY1_CLK));
}

static void MyJoystick::Rotary1Interrupt() {
  static unsigned long lastRotary1InterruptTime = 0;
  unsigned long currentTime = millis();
  
  if(currentTime - lastRotary1InterruptTime < DEBOUNCE_DELAY_MS) {
    return;
  }
  rotary1Direction = digitalRead(ROTARY1_DT) == LOW ? DIRECTION_LEFT : DIRECTION_RIGHT;    
  lastRotary1InterruptTime = currentTime;
}

void MyJoystick::UpdateRotary1Mode() {
  currentRotary1SwitchState = !digitalRead(ROTARY1_SW);
  if(previousRotary1SwitchState != currentRotary1SwitchState) {    
    if(currentRotary1SwitchState) {
      IncreaseRotary1Mode();
    }
    previousRotary1SwitchState = currentRotary1SwitchState;    
  }
}

void MyJoystick::IncreaseRotary1Mode() {
  if(++currentRotary1Mode >= rotaryModeCount)
    currentRotary1Mode = 0;
#ifdef DEBUG_SERIAL  
  PrintInputValue("Rotary1Mode", currentRotary1Mode + 1);
#endif  
}

int MyJoystick::GetRotary1ButtonForCurrentMode() {
  if(rotary1Direction == DIRECTION_NONE)
    return NO_BUTTON;      
  return rotarty1Buttons[currentRotary1Mode * 2 + rotary1Direction];
}

void MyJoystick::ReadRotary1() {
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

void MyJoystick::ReadKeyPad1() {
  int newButton = ReadKeyPadButton(KEYPAD1);
  if(newButton != currentKeyPad1Button) {
    ReleaseCurrentKeyPad1Button(currentKeyPad1Button);
    PressKeyPad1Button(newButton);    
  }
}

int MyJoystick::ReadKeyPadButton(int keyPadId) {  
  return ConvertAnalogToKeyPadButton(analogRead(keyPadId));
}

void MyJoystick::ReleaseCurrentKeyPad1Button(int button) {
  if(button != NO_BUTTON) {
    Joystick.setButton(button, LOW);
    updateJoystick = true;
#ifdef DEBUG_SERIAL 
    PrintInputValue("KeyPad1Release", button);
#endif      
  }
  currentKeyPad1Button = NO_BUTTON;
}

void MyJoystick::PressKeyPad1Button(int button) {
  if(button != NO_BUTTON) {  
    Joystick.setButton(button, HIGH);
    updateJoystick = true;    
#ifdef DEBUG_SERIAL 
    PrintInputValue("KeyPad1Press", button);
#endif  
  }
  currentKeyPad1Button = button;
}

void MyJoystick::ReadSlider1() {
  int newValue = ReadSlider(SLIDER1_INDEX, SLIDER1_PIN);
  if(newValue != NO_VALUE)
    Joystick.setRxAxis(newValue);
}

void MyJoystick::ReadSlider2() {
  int newValue = ReadSlider(SLIDER2_INDEX, SLIDER2_PIN);
  if(newValue != NO_VALUE)
    Joystick.setRyAxis(newValue);
}

int MyJoystick::ReadSlider(int sliderIndex, int sliderPin) {
  int newSliderValue = ReadSliderValue(sliderPin);
  if(newSliderValue != currentSliderValue[sliderIndex]) {
    updateJoystick = true;
    currentSliderValue[sliderIndex] = newSliderValue;
#ifdef DEBUG_SERIAL   
    char* slider = "SliderX";
    slider[6] = (char) (sliderIndex + 49);
    PrintInputValue(slider, newSliderValue);
#endif
    return newSliderValue;
  }
  return NO_VALUE;
}

int MyJoystick::ReadSliderValue(int sliderPin) {
  return ConvertAnalogToSliderValue(analogRead(sliderPin));
}

int MyJoystick::ConvertAnalogToKeyPadButton(int analogValue) {
  if(analogValue <= 100)
    return NO_BUTTON;
    
  int buttonIndex=0;
  do {
  } while(analogValue > keyPadResistorValues[buttonIndex++]);  
  
  return keyPadButtonIds[buttonIndex - 2];
}

int MyJoystick::ConvertAnalogToSliderValue(int analogValue) {
  return map(analogValue, 0, 1023, SLIDER_MIN, SLIDER_MAX);  
}

void MyJoystick::PrintInputValue(const char* label, int value) {   
  Serial.print(label);
  Serial.print(": ");
  Serial.println(value);
}
