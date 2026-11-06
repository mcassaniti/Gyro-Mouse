#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <Preferences.h>

#include <esp32-hal-rgb-led.h>

// --- Hardware Pin Definitions ---
#define PIN_TOUCH     6    // Capacitive touch pin (copper foil tape) - Clutch
#define PIN_TOUCH_BTN4 1   // Capacitive touch pin for Mouse Button 4 (Back)
#define PIN_TOUCH_BTN5 2   // Capacitive touch pin for Mouse Button 5 (Forward)
#define PIN_LED       48   // Status LED (WS2812 RGB on ESP32-S3 SuperMini)
#define PIN_I2C_SDA   4    // I2C Data
#define PIN_I2C_SCL   5    // I2C Clock

// Universal Status LED Control (supports both WS2812 RGB and standard GPIO LEDs)
inline void setStatusLED(bool on, uint8_t r = 0, uint8_t g = 64, uint8_t b = 0) {
  if (on) {
    rgbLedWrite(PIN_LED, r, g, b);
    digitalWrite(PIN_LED, HIGH);
  } else {
    rgbLedWrite(PIN_LED, 0, 0, 0);
    digitalWrite(PIN_LED, LOW);
  }
}

// --- Config Structure with Defaults ---
struct AppConfig {
  // Motion & Sensitivity Tuning
  float maxTilt          = 3.5f;     // Max tilt degrees to reach top speed
  float maxSpeed         = 1000.0f;  // Max cursor velocity in pixels/second (at 100 Hz / 10ms frame)
  float deadzone         = 0.35f;    // Tilt degrees ignored to prevent tremor jitter
  int   settleDelay      = 50;      // ms to wait after thumb contact before moving
  
  // Axis Inversion
  bool  invertPitch      = false;   // Forward/up tilt moves cursor UP
  bool  invertRoll       = false;   // Right tilt moves cursor RIGHT
  
  // Touch Thresholds (Schmitt Trigger with 75% Hysteresis)
  int   touchHigh        = 150000;  // Threshold to engage touch (firm touch contact)
  int   touchLow         = 112500;  // Threshold to release touch (75% of touchHigh)
  bool  clutchOverride   = false;   // True = always move, ignore touch switch
  bool  enableTouchButtons = true;  // GPIO 1 (Btn 4) and GPIO 2 (Btn 5) active
  
  // Tare Zero-Point Offsets
  float pitchOffset      = 0.0f;
  float rollOffset       = 0.0f;
  
  // Telemetry
  int   telemetryInterval = 0;      // ms between telemetry lines (0 = disabled at startup)
};

class ConfigManager {
public:
  AppConfig config;

  void begin();
  void load();
  void save();
  void resetDefaults();
  void printSummary();

private:
  Preferences prefs;
};

extern ConfigManager Config;

#endif // CONFIG_H
