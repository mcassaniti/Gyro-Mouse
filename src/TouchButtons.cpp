#include "TouchButtons.h"
#include "MouseController.h"
#include "SerialCLI.h"

TouchButtons Buttons;

void TouchButtons::begin() {
  btn4.state = false;
  btn5.state = false;
  btn4.lastRaw = 0;
  btn5.lastRaw = 0;
  if (!Config.config.enableTouchButtons) {
    return;
  }
  btn4.lastRaw = touchRead(btn4.pin);
  btn5.lastRaw = touchRead(btn5.pin);
}

void TouchButtons::update() {
  if (!Config.config.enableTouchButtons) {
    return;
  }
  updateChannel(btn4);
  updateChannel(btn5);
}

void TouchButtons::updateChannel(TouchButtonChannel &ch) {
  int raw = touchRead(ch.pin);
  
  // Ignore hardware overflow / timeout on floating pins
  if (raw > 1000000 || raw <= 0) {
    return;
  }
  ch.lastRaw = raw;

  // Schmitt Trigger Hysteresis using Config thresholds
  if (ch.lastRaw >= Config.config.touchHigh) {
    if (!ch.state) {
      ch.state = true;
      ch.pressDownTime = millis();
      MousePad.pressButton(ch.hidButton);
      if (CLI.isTouchTelemetryEnabled()) {
        Serial.printf("[TOUCH EVENT] %s PRESSED -> Raw: %d (>= ON: %d)\n",
                      ch.hidButton == MOUSE_BACKWARD ? "Button 4" : "Button 5",
                      ch.lastRaw, Config.config.touchHigh);
      }
    }
  } else if (ch.lastRaw <= Config.config.touchLow) {
    if (ch.state) {
      ch.state = false;
      unsigned long holdDuration = millis() - ch.pressDownTime;
      MousePad.releaseButton(ch.hidButton);
      if (CLI.isTouchTelemetryEnabled()) {
        Serial.printf("[TOUCH EVENT] %s RELEASED -> Raw: %d (<= OFF: %d, held: %lu ms)\n",
                      ch.hidButton == MOUSE_BACKWARD ? "Button 4" : "Button 5",
                      ch.lastRaw, Config.config.touchLow, holdDuration);
      }
    }
  }
}

bool TouchButtons::isPressed(uint8_t hidButton) {
  if (hidButton == btn4.hidButton) return btn4.state;
  if (hidButton == btn5.hidButton) return btn5.state;
  return false;
}

int TouchButtons::getRawValue(uint8_t pin) {
  if (pin == btn4.pin) return btn4.lastRaw;
  if (pin == btn5.pin) return btn5.lastRaw;
  return 0;
}
