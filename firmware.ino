#include <Arduino.h>
#include "src/Config.h"
#include "src/TouchClutch.h"
#include "src/TouchButtons.h"
#include "src/MPUManager.h"
#include "src/MouseController.h"
#include "src/SerialCLI.h"

void setup() {
  // 1. Initialize Persistent Configuration from NVS Flash
  Config.begin();

  // 2. Initialize Status LED
  pinMode(PIN_LED, OUTPUT);
  setStatusLED(false);

  // 3. Initialize Interactive Serial CLI & USB Handshake
  CLI.begin();

  // 4. Initialize Hardware Touch Clutch, Touch Buttons & MPU-6050
  Clutch.begin();
  Buttons.begin();
  Gyro.begin();

  // 5. Initialize Native USB HID Mouse
  MousePad.begin();

  // 6. Perform initial Tare calibration on boot
  if (Gyro.isReady()) {
    delay(100);
    Gyro.calibrateTare("Boot Tare Baseline");
    Serial.println(F("[READY] Gyro Mouse Base initialized and running loop!\n"));
  }
}

void loop() {
  // 1. Update Touch Clutch State & Check Multi-Tap Gestures
  Clutch.update();
  if (Clutch.checkTripleTap()) {
    Serial.println(F("\n[EVENT] Triple-Tap Detected -> Re-Zeroing Gyro Tare..."));
    Gyro.calibrateTare("Triple-Tap Gesture");
    setStatusLED(true, 0, 64, 0); // Green confirmation flash
    delay(100);
  }

  // 2. Visual Clutch Status (Blue for 2x Sprint, Red for Normal Clutch, Off when disengaged)
  if (Clutch.isEngaged()) {
    if (Clutch.isSprint()) {
      setStatusLED(true, 0, 0, 64); // Blue (2x Sprint Speed)
    } else {
      setStatusLED(true, 48, 0, 0); // Red (1x Normal Speed)
    }
  } else {
    setStatusLED(false);            // Off
  }

  // 3. Update Capacitive Touch Buttons (GPIO 1 -> Btn 4, GPIO 2 -> Btn 5)
  Buttons.update();

  // 4. Update MPU Sensor Angles
  Gyro.update();

  // 5. Process Linear Mouse Motion
  MotionState state = MousePad.process(
    Gyro.getActivePitch(),
    Gyro.getActiveRoll(),
    Clutch.isEngaged(),
    Clutch.isSettling(),
    Clutch.isSprint()
  );

  // 5. Update Serial CLI, Telemetry Stream & Command Handlers
  CLI.update(state);

  // 6. Loop Stability (100 Hz Polling Rate)
  delay(10);
}
