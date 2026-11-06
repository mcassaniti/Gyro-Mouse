#include "SerialCLI.h"
#include <esp32-hal-tinyusb.h>
#include <soc/rtc_cntl_reg.h>

SerialCLI CLI;

void SerialCLI::begin() {
  Serial.begin(115200);
  Serial.setTxTimeoutMs(10); // Prevent blocking if no terminal connected

  unsigned long start = millis();
  while (!Serial && (millis() - start < 2500)) {
    delay(10);
  }

  printHelp();
}

void SerialCLI::update(MotionState state) {
  if (Serial) {
    if (!wasConnected) {
      wasConnected = true;
      printHelp();
    }
    while (Serial.available()) {
      handleCommand(Serial.read());
    }
  } else {
    wasConnected = false;
  }

  // Pending command sequence timeout (4 seconds)
  if (pendingChar != '\0' && (millis() - pendingTime > 4000)) {
    Serial.printf("[TIMEOUT] Incomplete command sequence: '%c'. Cancelled.\n", pendingChar);
    pendingChar = '\0';
  }

  // Periodic Telemetry Stream (Motion: 'i')
  int interval = Config.config.telemetryInterval;
  if (interval > 0 && (millis() - lastTelemetryTime >= (unsigned long)interval)) {
    lastTelemetryTime = millis();

    const char* stateStr = "DEADZONE";
    switch (state) {
      case STATE_MOVING:          stateStr = "MOVING"; break;
      case STATE_DEADZONE:        stateStr = "DEADZONE"; break;
      case STATE_SETTLING:        stateStr = "SETTLING"; break;
      case STATE_FROZEN_MOVING:   stateStr = "FROZEN (MOVING)"; break;
      case STATE_FROZEN_DEADZONE: stateStr = "FROZEN (DEADZONE)"; break;
      case STATE_FROZEN_SETTLING: stateStr = "FROZEN (SETTLING)"; break;
    }

    int dX = (state == STATE_MOVING) ? MousePad.getLastMoveX() : MousePad.getTargetMoveX();
    int dY = (state == STATE_MOVING) ? MousePad.getLastMoveY() : MousePad.getTargetMoveY();

    Serial.printf("[MOTION] Raw:(P:%+5.1f°, R:%+5.1f°) | Active:(P:%+5.1f°, R:%+5.1f°) | Move:(dX:%+3d, dY:%+3d) [%s]\n",
                  Gyro.getRawPitch(), Gyro.getRawRoll(),
                  Gyro.getActivePitch(), Gyro.getActiveRoll(),
                  dX, dY,
                  stateStr);
  }
}

void SerialCLI::handleCommand(char c) {
  // Ignore whitespace and standalone newlines
  if (c == '\r' || c == '\n') {
    if (pendingChar != '\0') {
      pendingChar = '\0';
    }
    return;
  }
  if (c == ' ' || c == '\t') {
    return;
  }

  // State 1: No pending prefix character
  if (pendingChar == '\0') {
    // Immediate 1-character commands
    switch (c) {
      case 'h':
      case 'H':
      case '?':
        printHelp();
        return;

      case 'i':
      case 'I':
        if (Config.config.telemetryInterval > 0) {
          Config.config.telemetryInterval = 0;
          Serial.println(F("[MOTION] Live motion telemetry stream OFF. Send 'i' to toggle ON."));
        } else {
          Config.config.telemetryInterval = 200;
          Serial.println(F("[MOTION] Live motion telemetry stream ON (200ms). Send 'i' to toggle OFF."));
        }
        return;

      case 'j':
      case 'J':
        touchTelemetry = !touchTelemetry;
        if (touchTelemetry) {
          Clutch.resetPeaks();
          Serial.println(F("[TOUCH] Live touch event logging ON (Outputs on Press/Release). Send 'j' to toggle OFF."));
        } else {
          Serial.println(F("[TOUCH] Live touch event logging OFF. Send 'j' to toggle ON."));
        }
        return;

      case 'z':
      case 'Z':
        Gyro.calibrateTare("Serial Command (z)");
        return;

      case 'c':
      case 'C':
        Config.config.clutchOverride = !Config.config.clutchOverride;
        Serial.printf("[CONFIG] Clutch Override: %s (%s)\n",
                      Config.config.clutchOverride ? "ACTIVE (Bypassing touch switch)" : "DISABLED (Touch Required)",
                      Config.config.clutchOverride ? "Mouse moves freely on tilt" : "Thumb must touch tape");
        return;

      case 'b':
      case 'B':
        Config.config.enableTouchButtons = !Config.config.enableTouchButtons;
        if (Config.config.enableTouchButtons) {
          Buttons.begin();
          Serial.println(F("[CONFIG] Touch Buttons 4 & 5: ENABLED (GPIO 1/2 active)"));
        } else {
          Serial.println(F("[CONFIG] Touch Buttons 4 & 5: DISABLED (Ignoring floating GPIO 1/2)"));
        }
        return;

      case 'x':
      case 'X':
        Config.config.invertRoll = !Config.config.invertRoll;
        Serial.printf("[CONFIG] Roll (X-axis) Invert: %s\n", Config.config.invertRoll ? "INVERTED" : "NORMAL");
        return;

      case 'y':
      case 'Y':
        Config.config.invertPitch = !Config.config.invertPitch;
        Serial.printf("[CONFIG] Pitch (Y-axis) Invert: %s\n", Config.config.invertPitch ? "INVERTED" : "NORMAL");
        return;

      case 'P':
        Config.config.telemetryInterval = 0;
        touchTelemetry = false;
        wasConnected = false;
        Serial.println(F("\n[SYSTEM] Entering ESP32-S3 ROM Bootloader / Programming Mode...\x04"));
        Serial.flush();
        delay(150);
#if CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32S2
        REG_WRITE(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT);
        usb_persist_restart(RESTART_BOOTLOADER);
#else
        esp_restart();
#endif
        return;

      case 'q':
      case 'Q':
        Config.config.telemetryInterval = 0;
        touchTelemetry = false;
        wasConnected = false;
        Serial.println(F("\n[SESSION] Serial session closed by device. Goodbye!\x04"));
        Serial.flush();
        return;

      // 2-character prefixes
      case 'd': case 'D':
      case 't': case 'T':
      case 's': case 'S':
      case 'u': case 'U':
        pendingChar = (char)tolower(c);
        pendingTime = millis();
        return;

      case '+': case '=':
        pendingChar = '+';
        pendingTime = millis();
        return;

      case '-': case '_':
        pendingChar = '-';
        pendingTime = millis();
        return;

      default:
        Serial.printf("[ERROR] Unknown command: '%c'. Type 'h' or '?' for help.\n", c);
        return;
    }
  }

  // State 2: Processing second character of 2-char sequence
  char first = pendingChar;
  char second = c;
  pendingChar = '\0'; // Clear prefix state

  if (first == 'd') {
    if (second == '+' || second == '=') {
      Config.config.deadzone += 0.05f;
      if (Config.config.deadzone > 1.50f) Config.config.deadzone = 1.50f;
      Serial.printf("[CONFIG] (d+) Deadzone increased to: %.2f deg\n", Config.config.deadzone);
    } else if (second == '-' || second == '_') {
      Config.config.deadzone -= 0.05f;
      if (Config.config.deadzone < 0.05f) Config.config.deadzone = 0.05f;
      Serial.printf("[CONFIG] (d-) Deadzone decreased to: %.2f deg\n", Config.config.deadzone);
    } else {
      Serial.printf("[ERROR] Invalid combination: \"%c%c\". Expected 'd+' or 'd-'.\n", first, second);
    }
  } else if (first == 't') {
    if (second == '+' || second == '=') {
      Config.config.maxTilt += 0.05f;
      if (Config.config.maxTilt > 6.50f) Config.config.maxTilt = 6.50f;
      Serial.printf("[CONFIG] (t+) Max Tilt Angle increased to: %.2f deg (Lower sensitivity)\n", Config.config.maxTilt);
    } else if (second == '-' || second == '_') {
      Config.config.maxTilt -= 0.05f;
      if (Config.config.maxTilt < 0.80f) Config.config.maxTilt = 0.80f;
      Serial.printf("[CONFIG] (t-) Max Tilt Angle decreased to: %.2f deg (Higher sensitivity)\n", Config.config.maxTilt);
    } else {
      Serial.printf("[ERROR] Invalid combination: \"%c%c\". Expected 't+' or 't-'.\n", first, second);
    }
  } else if (first == 's') {
    if (second == '+' || second == '=') {
      Config.config.maxSpeed += 50.0f;
      if (Config.config.maxSpeed > 6000.0f) Config.config.maxSpeed = 6000.0f;
      Serial.printf("[CONFIG] (s+) Max Speed increased to: %.0f px/sec\n", Config.config.maxSpeed);
    } else if (second == '-' || second == '_') {
      Config.config.maxSpeed -= 50.0f;
      if (Config.config.maxSpeed < 100.0f) Config.config.maxSpeed = 100.0f;
      Serial.printf("[CONFIG] (s-) Max Speed decreased to: %.0f px/sec\n", Config.config.maxSpeed);
    } else {
      Serial.printf("[ERROR] Invalid combination: \"%c%c\". Expected 's+' or 's-'.\n", first, second);
    }
  } else if (first == 'u') {
    if (second == '+' || second == '=') {
      Config.config.touchHigh += 5000;
      if (Config.config.touchHigh > 300000) Config.config.touchHigh = 300000;
      Config.config.touchLow = (int)(Config.config.touchHigh * 0.75f);
      Serial.printf("[CONFIG] (u+) Touch Thresholds increased -> ON: %d | OFF: %d (75%%)\n", Config.config.touchHigh, Config.config.touchLow);
    } else if (second == '-' || second == '_') {
      Config.config.touchHigh -= 5000;
      if (Config.config.touchHigh < 25000) Config.config.touchHigh = 25000;
      Config.config.touchLow = (int)(Config.config.touchHigh * 0.75f);
      Serial.printf("[CONFIG] (u-) Touch Thresholds decreased -> ON: %d | OFF: %d (75%%)\n", Config.config.touchHigh, Config.config.touchLow);
    } else {
      Serial.printf("[ERROR] Invalid combination: \"%c%c\". Expected 'u+' or 'u-'.\n", first, second);
    }
  } else if (first == '+') {
    if (second == '+' || second == '=') {
      Config.save();
      Serial.println(F("[CONFIG] (++) All configuration settings saved to NVS Flash!"));
    } else {
      Serial.printf("[ERROR] Invalid combination: \"%c%c\". Did you mean '++' to save?\n", first, second);
    }
  } else if (first == '-') {
    if (second == '-' || second == '_') {
      Config.resetDefaults();
      Serial.println(F("[CONFIG] (--) Configuration reset to factory defaults and saved to NVS!"));
    } else {
      Serial.printf("[ERROR] Invalid combination: \"%c%c\". Did you mean '--' to factory reset?\n", first, second);
    }
  }
}

void SerialCLI::printHelp() {
  Serial.println();
  Serial.println(F("================================================================="));
  Serial.println(F("             GYRO MOUSE BASE: ESP32-S3 GYRO-MOUSE                "));
  Serial.println(F("================================================================="));
  Serial.println(F("  COMMAND  | DESCRIPTION"));
  Serial.println(F(" ----------+-----------------------------------------------------"));
  Serial.println(F("   d+ / d- | Adjust Deadzone Threshold (0.05° - 1.50°)"));
  Serial.println(F("   t+ / t- | Adjust Max Tilt Angle / Sensitivity (0.80° - 6.50°)"));
  Serial.println(F("   s+ / s- | Adjust Max Cursor Velocity (100 - 6000 px/sec)"));
  Serial.println(F("   u+ / u- | Adjust Touch Thresholds (75% Linked Hysteresis)"));
  Serial.println(F("     ++    | Save Current Configuration to NVS Flash"));
  Serial.println(F("     --    | Factory Reset Configuration to Defaults"));
  Serial.println(F("      i    | Toggle Live Motion Telemetry Stream (ON / OFF)"));
  Serial.println(F("      j    | Toggle Live Touch Event Logging (Press / Release)"));
  Serial.println(F("      z    | Zero / Tare Gyro Baseline (RAM or Triple-Tap)"));
  Serial.println(F("      c    | Toggle Clutch Override (Always Move vs Touch Required)"));
  Serial.println(F("      b    | Toggle Touch Buttons 4/5 (Enable/Disable GPIO 1/2)"));
  Serial.println(F("      x    | Toggle Roll (X-Axis) Inversion (Normal / Inverted)"));
  Serial.println(F("      y    | Toggle Pitch (Y-Axis) Inversion (Normal / Inverted)"));
  Serial.println(F("      P    | Enter ROM Bootloader / Programming Mode (Flash Ready)"));
  Serial.println(F("   h / ?   | Display this Help & Configuration Menu"));
  Serial.println(F("     q     | Quit Serial Monitor Session (Exit to shell)"));
  Serial.println(F("================================================================="));
  Serial.println(F(" GESTURES: Double-Tap & Hold = 2x Sprint | Triple-Tap = Tare"));
  Serial.println(F("================================================================="));
  Config.printSummary();
}

void SerialCLI::printSensorReport() {
  Serial.println(F("\n--- SENSOR DIAGNOSTIC REPORT ---"));
  Serial.printf("  Chip Acceleration (g)   : X: %+6.2f, Y: %+6.2f, Z: %+6.2f\n", Gyro.getAccX(), Gyro.getAccY(), Gyro.getAccZ());
  Serial.printf("  Raw Angles (90° Mapped) : Pitch: %+6.2f°, Roll: %+6.2f°\n", Gyro.getRawPitch(), Gyro.getRawRoll());
  Serial.printf("  Active Angles (Offset)  : Pitch: %+6.2f°, Roll: %+6.2f°\n", Gyro.getActivePitch(), Gyro.getActiveRoll());
  Serial.printf("  Deadzone Threshold      : %.2f deg\n", Config.config.deadzone);
  Serial.printf("  Temperature             : %.2f °C\n", Gyro.getTemp());
  Serial.printf("  Touch Thresholds        : ON >= %d | OFF <= %d (75%% hysteresis)\n", Config.config.touchHigh, Config.config.touchLow);
  Serial.printf("  Clutch (GPIO 6)         : Raw: %d [Observed Min: %d, Max: %d] (%s%s)\n",
                Clutch.getRawValue(),
                Clutch.getMinRaw(),
                Clutch.getMaxRaw(),
                Clutch.isEngaged() ? "TOUCHED" : "OPEN",
                Clutch.isSprint() ? " [2x SPRINT]" : "");
  Serial.printf("  Btn 4 (GPIO 1)          : Raw: %d (%s)\n",
                Buttons.getRawValue(PIN_TOUCH_BTN4), Buttons.isPressed(MOUSE_BACKWARD) ? "PRESSED" : "OPEN");
  Serial.printf("  Btn 5 (GPIO 2)          : Raw: %d (%s)\n",
                Buttons.getRawValue(PIN_TOUCH_BTN5), Buttons.isPressed(MOUSE_FORWARD) ? "PRESSED" : "OPEN");
  Serial.println(F("--------------------------------\n"));
}
