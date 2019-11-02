#define DELAY_TIME 50
#define LED_PIN 13
#define SLIDER1 A0
#define SLIDER2 A2
#define KEYPAD A1

const int keyPadResistorValues[] = {400, 500, 530, 560, 590, 630, 670, 720, 770, 830, 910, 1000};
const int keyPadButtonIds[] = {12, 0, 11, 9, 8, 7, 6, 5, 4, 3, 2, 1};

void setup() {
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(19200);
}

void loop() {
  unsigned long startTime = micros();
  ReportKeyPad();
  ReportSlider1();
  digitalWrite(LED_PIN, HIGH);
  delay(DELAY_TIME);
  digitalWrite(LED_PIN, LOW);
  unsigned long stopTime = micros();

  // skip time overflow  
  if(stopTime > startTime) {
    Serial.print(stopTime - startTime);
    Serial.println("us");
  }
}

void ReportKeyPad() {
  const int keyPadButton = convertAnalogToKeypadButton(analogRead(KEYPAD));
  if(keyPadButton >= 0)
    printSensorValue("Keypad", keyPadButton);
}

void ReportSlider1() {
  printSensorValue("Slider1", convertAnalogToSlieder(analogRead(SLIDER1)));
}

int convertAnalogToKeypadButton(int value) {
  if(value <= 100)
    return -1;

  int buttonIndex=0;
  do {
  } while(value > keyPadResistorValues[buttonIndex++]);  
  
  return keyPadButtonIds[buttonIndex - 2];
}

inline int convertAnalogToSlieder(int value) {
  return map(value, 0, 1023, 0, 100);
}

inline void printSensorValue(const char* label, int value) {   
  Serial.print(label);
  Serial.print(": ");
  Serial.println(value);
}
