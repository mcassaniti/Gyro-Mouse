#ifndef TOUCH_BUTTONS_H
#define TOUCH_BUTTONS_H

#include <Arduino.h>
#include <USBHIDMouse.h>
#include "Config.h"

struct TouchButtonChannel {
  uint8_t pin;
  uint8_t hidButton;
  bool state;
  int lastRaw;
  unsigned long pressDownTime;
};

class TouchButtons {
public:
  void begin();
  void update();

  bool isPressed(uint8_t hidButton);
  int getRawValue(uint8_t pin);

private:
  TouchButtonChannel btn4 = { PIN_TOUCH_BTN4, MOUSE_BACKWARD, false, 0, 0 };
  TouchButtonChannel btn5 = { PIN_TOUCH_BTN5, MOUSE_FORWARD,  false, 0, 0 };

  void updateChannel(TouchButtonChannel &ch);
};

extern TouchButtons Buttons;

#endif // TOUCH_BUTTONS_H
