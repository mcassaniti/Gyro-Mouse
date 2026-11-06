#ifndef MOUSE_CONTROLLER_H
#define MOUSE_CONTROLLER_H

#include <Arduino.h>
#include <USB.h>
#include <USBHIDMouse.h>
#include "Config.h"

enum MotionState {
  STATE_FROZEN_DEADZONE,
  STATE_FROZEN_MOVING,
  STATE_FROZEN_SETTLING,
  STATE_DEADZONE,
  STATE_MOVING,
  STATE_SETTLING
};

class MouseController {
public:
  void begin();
  MotionState process(float activePitch, float activeRoll, bool isTouched, bool isSettling, bool isSprint = false);
  
  int getLastMoveX() { return lastMoveX; }
  int getLastMoveY() { return lastMoveY; }
  int getTargetMoveX() { return targetMoveX; }
  int getTargetMoveY() { return targetMoveY; }

  void pressButton(uint8_t b);
  void releaseButton(uint8_t b);
  bool isButtonPressed(uint8_t b);

private:
  USBHIDMouse mouse;
  int lastMoveX = 0;
  int lastMoveY = 0;
  int targetMoveX = 0;
  int targetMoveY = 0;
  
  int calculateVelocity(float angle, float speedMultiplier = 1.0f);
};

extern MouseController MousePad;

#endif // MOUSE_CONTROLLER_H
