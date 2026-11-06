#include "MouseController.h"

MouseController MousePad;

void MouseController::begin() {
  mouse.begin();
  USB.begin();
}

void MouseController::pressButton(uint8_t b) {
  mouse.press(b);
}

void MouseController::releaseButton(uint8_t b) {
  mouse.release(b);
}

bool MouseController::isButtonPressed(uint8_t b) {
  return mouse.isPressed(b);
}

MotionState MouseController::process(float activePitch, float activeRoll, bool isTouched, bool isSettling, bool isSprint) {
  lastMoveX = 0;
  lastMoveY = 0;

  // Apply axis inversion
  float dirRoll  = Config.config.invertRoll  ? -activeRoll  : activeRoll;
  float dirPitch = Config.config.invertPitch ? -activePitch : activePitch;

  float multiplier = isSprint ? 2.0f : 1.0f;
  targetMoveX = calculateVelocity(dirRoll, multiplier);
  targetMoveY = calculateVelocity(dirPitch, multiplier);

  if (!isTouched) {
    if (isSettling) return STATE_FROZEN_SETTLING;
    if (targetMoveX != 0 || targetMoveY != 0) return STATE_FROZEN_MOVING;
    return STATE_FROZEN_DEADZONE;
  }

  if (isSettling) {
    return STATE_SETTLING;
  }

  lastMoveX = targetMoveX;
  lastMoveY = targetMoveY;

  if (lastMoveX != 0 || lastMoveY != 0) {
    mouse.move(lastMoveX, lastMoveY, 0);
    return STATE_MOVING;
  }

  return STATE_DEADZONE;
}

int MouseController::calculateVelocity(float angle, float speedMultiplier) {
  float maxT = Config.config.maxTilt;
  float maxS = Config.config.maxSpeed * speedMultiplier;
  float dead = Config.config.deadzone;

  // Constrain angle to software limits
  if (angle > maxT) angle = maxT;
  if (angle < -maxT) angle = -maxT;

  float absAngle = abs(angle);

  // Deadzone filter: Ignore micro-tremors
  if (absAngle <= dead) return 0;

  // Active travel normalization (from deadzone to maxTilt)
  float activeRange = maxT - dead;
  if (activeRange < 0.05f) activeRange = 0.05f;

  float normalized = (absAngle - dead) / activeRange;
  if (normalized > 1.0f) normalized = 1.0f;

  // Pure Linear Speed Mapping: Convert px/sec to px/frame (100 Hz / 10ms frame)
  float speed = (normalized * maxS) / 100.0f;

  // Preserve direction sign
  if (angle < 0) speed = -speed;

  return (int)roundf(speed);
}
