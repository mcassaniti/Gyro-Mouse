#ifndef SERIAL_CLI_H
#define SERIAL_CLI_H

#include <Arduino.h>
#include "Config.h"
#include "TouchClutch.h"
#include "TouchButtons.h"
#include "MPUManager.h"
#include "MouseController.h"

class SerialCLI {
public:
  void begin();
  void update(MotionState state);
  
  void printHelp();
  void printSensorReport();
  bool isTouchTelemetryEnabled() { return touchTelemetry; }

private:
  unsigned long lastTelemetryTime = 0;
  bool touchTelemetry = false;
  bool wasConnected = false;
  char pendingChar = '\0';
  unsigned long pendingTime = 0;
  
  void handleCommand(char c);
};

extern SerialCLI CLI;

#endif // SERIAL_CLI_H
