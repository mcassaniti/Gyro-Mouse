#include "Config.h"

ConfigManager Config;

void ConfigManager::begin() {
  prefs.begin("gyrobase", false); // Open namespace in read/write mode
  load();
  config.telemetryInterval = 0; // Always start with live telemetry streaming OFF
}

void ConfigManager::load() {
  config.maxTilt           = prefs.getFloat("max_tilt", 3.5f);
  config.maxSpeed          = prefs.getFloat("max_speed", 1000.0f);
  config.deadzone          = prefs.getFloat("deadzone", 0.35f);
  config.settleDelay       = prefs.getInt("settle_ms", 50);
  config.invertPitch       = prefs.getBool("inv_pitch", false);
  config.invertRoll        = prefs.getBool("inv_roll", false);
  config.touchHigh         = prefs.getInt("touch_hi", 150000);
  config.touchLow          = (int)(config.touchHigh * 0.75f);
  config.clutchOverride    = prefs.getBool("clutch_ovr", false);
  config.enableTouchButtons = prefs.getBool("btn_touch", true);
  config.pitchOffset       = 0.0f; // Calibrated fresh on boot/demand (RAM only)
  config.rollOffset        = 0.0f; // Calibrated fresh on boot/demand (RAM only)
  config.telemetryInterval = 0;
}

void ConfigManager::save() {
  config.touchLow = (int)(config.touchHigh * 0.75f);
  prefs.putFloat("max_tilt", config.maxTilt);
  prefs.putFloat("max_speed", config.maxSpeed);
  prefs.putFloat("deadzone", config.deadzone);
  prefs.putInt("settle_ms", config.settleDelay);
  prefs.putBool("inv_pitch", config.invertPitch);
  prefs.putBool("inv_roll", config.invertRoll);
  prefs.putInt("touch_hi", config.touchHigh);
  prefs.putInt("touch_lo", config.touchLow);
  prefs.putBool("clutch_ovr", config.clutchOverride);
  prefs.putBool("btn_touch", config.enableTouchButtons);
  // pitchOffset and rollOffset are kept in RAM to avoid flash wear & posture staleness
}

void ConfigManager::resetDefaults() {
  prefs.clear(); // Clear all keys in namespace
  config = AppConfig(); // Reset to struct defaults
  save();
}

void ConfigManager::printSummary() {
  Serial.println(F("\n--- CURRENT CONFIGURATION ---"));
  Serial.printf("  Max Speed (s)              : %.0f px/sec\n", config.maxSpeed);
  Serial.printf("  Max Tilt Sensitivity (t)   : %.1f deg\n", config.maxTilt);
  Serial.printf("  Deadzone (d)               : %.2f deg\n", config.deadzone);
  Serial.printf("  Settling Delay             : %d ms\n", config.settleDelay);
  Serial.printf("  Invert Pitch (y)           : %s\n", config.invertPitch ? "YES (Inverted)" : "NO (Normal: Up=Up)");
  Serial.printf("  Invert Roll (x)            : %s\n", config.invertRoll ? "YES (Inverted)" : "NO (Normal: Right=Right)");
  Serial.printf("  Touch Thresholds (u)       : ON >= %d | OFF <= %d\n", config.touchHigh, config.touchLow);
  Serial.printf("  Clutch Override (c)        : %s\n", config.clutchOverride ? "ACTIVE (Bypassing touch switch)" : "DISABLED (Touch Required)");
  Serial.printf("  Touch Buttons 4/5 (b)      : %s\n", config.enableTouchButtons ? "ENABLED (GPIO 1/2 Active)" : "DISABLED (Floating / Not wired)");
  Serial.printf("  Tare Offsets (z)           : Pitch: %+6.2f deg | Roll: %+6.2f deg\n", config.pitchOffset, config.rollOffset);
  Serial.printf("  Telemetry Stream (i)       : %d ms (%s)\n", config.telemetryInterval, config.telemetryInterval > 0 ? "Streaming" : "Disabled");
  Serial.println(F("----------------------------------\n"));
}
