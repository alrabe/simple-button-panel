#include "MySettings.h"
#include "MyJoystick.h"
#include "MyKeyboard.h"

MyJoystick joystick;
MyKeyboard keyboard;

void setup() {
  SetupPins();
  SetupDebug();
  joystick.EnableInterrupts();
}

void loop() {
  joystick.DisableInterrupts();
  unsigned long currentExecutionTime = millis();
  
  joystick.Update();
  Blink();
  joystick.EnableInterrupts();
  
  ReportTiming();
  joystick.Report();

  DelayNextExecution(currentExecutionTime);
}

inline void DelayNextExecution(unsigned long lastExecution) {  
  unsigned long nextExecution = lastExecution + DELAY_TIME_MS;
  while(millis() < nextExecution) {
    // prevent interrupts from shortcutting the delay
    delay(nextExecution - millis());
  }
}

void ReportTiming() {
#ifdef DEBUG_TIME  
  static unsigned long lastReportTime = 0;
  unsigned long currentTime = millis();
  
  if(currentTime > lastReportTime) {
    Serial.print(currentTime - lastReportTime);
    Serial.println(" ms");
  }
  lastReportTime = currentTime;
#endif  
}

void Blink() {
  static unsigned long lastBlinkTime = 0;
  static bool ledState = LOW;
  
  unsigned long currentTime = millis();
  
  if(currentTime - lastBlinkTime < BLINK_TIME_MS) {
    return;
  }
  ledState = !ledState;
  digitalWrite(LED_BUILTIN, ledState);
  lastBlinkTime = currentTime;  
}

void SetupPins() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(ROTARY1_SW, INPUT_PULLUP);
  pinMode(ROTARY1_CLK, INPUT);
  pinMode(ROTARY1_DT, INPUT);
}

void SetupDebug() {
  #ifdef DEBUG_SERIAL  
  Serial.begin(19200);
#else if DEBUG_TIME
  Serial.begin(19200);
#endif
}
