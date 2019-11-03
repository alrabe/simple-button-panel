#define DELAY_TIME 50
#define LED 13
#define SLIDER1 A0
#define SLIDER2 A2
#define KEYPAD A1

const int keyPadResistorValues[] = {400, 500, 530, 560, 590, 630, 670, 720, 770, 830, 910, 1000};
const int keyPadButtonIds[] = {9, 5, 1, 10, 6, 2, 11, 7, 3, 12, 8, 4};

void setup() {
  pinMode(LED, OUTPUT);
  Serial.begin(19200);
}

void loop() {
  unsigned long startTime = micros();
  ReportKeyPad();
  ReportSlider1();
  DimLed();
  delay(DELAY_TIME);  
  //ReportTime(startTime, micros());
}

void ReportTime(long startTime, long stopTime) {
  // skip time overflow  
  if(stopTime > startTime) {
    Serial.print(stopTime - startTime);
    Serial.println("us");
  }  
}

void DimLed() {
  int brightness = map(analogRead(SLIDER1), 0, 1023, 0, 255);
  analogWrite(LED, brightness);
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
