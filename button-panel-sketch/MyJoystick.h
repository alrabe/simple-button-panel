#ifndef MyJoystick_h
#define MyJoystick_h

class MyJoystick {
  public:
    MyJoystick();
    void Update();
    void Report();
    void EnableInterrupts();
    void DisableInterrupts();
    
  private:
    void ReadRotary1();
    void static Rotary1Interrupt();
    void UpdateRotary1Mode();
    void IncreaseRotary1Mode();
    int GetRotary1ButtonForCurrentMode();    

    void ReadKeyPad1();
    int ReadKeyPadButton(int keyPadId);
    void ReleaseCurrentKeyPad1Button(int button);
    void PressKeyPad1Button(int button);

    void ReadSlider1();
    void ReadSlider2();
    int ReadSlider(int sliderIndex, int sliderPin);
    int ReadSliderValue(int sliderPin);
    int ConvertAnalogToKeyPadButton(int analogValue);
    int ConvertAnalogToSliderValue(int analogValue);

    void PrintInputValue(const char* label, int value);
};

#endif
