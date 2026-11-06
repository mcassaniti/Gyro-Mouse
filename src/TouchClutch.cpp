#include "TouchClutch.h"
#include "SerialCLI.h"

TouchClutch Clutch;

void TouchClutch::begin() {
  lastRawValue = touchRead(PIN_TOUCH);
  minRaw = lastRawValue;
  maxRaw = lastRawValue;
  rawTouchedState = false;
  prevTouchedState = false;
  settlingActive = false;
  tapCount = 0;
  isSprintActive = false;
  tripleTapFired = false;
}

void TouchClutch::update() {
  lastRawValue = touchRead(PIN_TOUCH);
  if (lastRawValue > 0 && lastRawValue < 1000000) {
    if (lastRawValue < minRaw) minRaw = lastRawValue;
    if (lastRawValue > maxRaw) maxRaw = lastRawValue;
  }

  // Schmitt Trigger Hysteresis
  if (lastRawValue >= Config.config.touchHigh) {
    rawTouchedState = true;
  } else if (lastRawValue <= Config.config.touchLow) {
    rawTouchedState = false;
  }

  unsigned long now = millis();

  // Edge detection: Just touched down
  if (rawTouchedState && !prevTouchedState) {
    touchDownTime = now;
    settlingActive = true;
    if (CLI.isTouchTelemetryEnabled()) {
      Serial.printf("[TOUCH EVENT] Clutch ENGAGED -> Raw: %d (>= ON: %d)\n", lastRawValue, Config.config.touchHigh);
    }

    // Reset tap sequence if too long since previous release
    if (tapCount > 0 && (now - lastReleaseTime > tapGapTimeout)) {
      tapCount = 0;
    }

    if (tapCount == 0) {
      isSprintActive = false;
    } else if (tapCount == 1) {
      // 2nd touch down: Momentary 2x sprint mode engaged while held
      isSprintActive = true;
    } else if (tapCount >= 2) {
      // 3rd touch down: Triple-tap gesture detected (Tare)
      tripleTapFired = true;
      tapCount = 0;
      isSprintActive = false;
      settlingActive = false;
    }
  }

  // Edge detection: Just released (touch up)
  if (!rawTouchedState && prevTouchedState) {
    unsigned long holdDuration = now - touchDownTime;
    isSprintActive = false;
    settlingActive = false;
    if (CLI.isTouchTelemetryEnabled()) {
      Serial.printf("[TOUCH EVENT] Clutch RELEASED -> Raw: %d (<= OFF: %d, held: %lu ms)\n", lastRawValue, Config.config.touchLow, holdDuration);
    }

    if (holdDuration < tapMaxDuration) {
      // Quick tap completed
      tapCount++;
      lastReleaseTime = now;
      if (tapCount >= 3) {
        tapCount = 0;
      }
    } else {
      // Long press / normal hold ended -> clear tap sequence
      tapCount = 0;
    }
  }

  // Timeout for incomplete multi-tap sequence while untouched
  if (!rawTouchedState && tapCount > 0 && (now - lastReleaseTime > tapGapTimeout)) {
    tapCount = 0;
  }

  // Settling window check
  if (settlingActive && (now - touchDownTime >= (unsigned long)Config.config.settleDelay)) {
    settlingActive = false;
  }

  prevTouchedState = rawTouchedState;
}

bool TouchClutch::isEngaged() {
  return Config.config.clutchOverride || rawTouchedState;
}

bool TouchClutch::isSettling() {
  return settlingActive && !Config.config.clutchOverride;
}

bool TouchClutch::isSprint() {
  return isSprintActive;
}

bool TouchClutch::checkTripleTap() {
  if (tripleTapFired) {
    tripleTapFired = false;
    return true;
  }
  return false;
}

int TouchClutch::getRawValue() {
  return lastRawValue;
}
