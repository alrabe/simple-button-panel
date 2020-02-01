#ifndef mySettings_h
#define mySettings_h

// *** DEBUG settings ***
#define DEBUG_SERIAL
//#define DEBUG_TIME
//#define DEBUG_SKIP_JOYSTICK_UPDATE
//#define DEBUG_SKIP_BEFORE_ROTARYMODE1 // do not send joystick events until rotary1 is clicked once

// *** Global settings ***
// connected pins
#define SLIDER1_PIN A0
#define SLIDER2_PIN A1
#define KEYPAD1 A2
#define ROTARY1_DT 6
#define ROTARY1_CLK 7   // need to support interrupts (1,2,3,7 on Leonardo)!
#define ROTARY1_SW 8

// internals
#define DEBOUNCE_DELAY_MS 5
#define DELAY_TIME_MS 40
#define BLINK_TIME_MS 250

#define NO_BUTTON -1
#define NO_VALUE -1
#define DIRECTION_LEFT 0
#define DIRECTION_RIGHT 1
#define DIRECTION_NONE -1

// stop slider axis flattering by downscaling resolution from 0<->1023 to 0<->341
#define SLIDER_MIN 0
#define SLIDER_MAX 341
#define SLIDER1_INDEX 0
#define SLIDER2_INDEX 1

#endif
