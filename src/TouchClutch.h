#ifndef TOUCH_CLUTCH_H
#define TOUCH_CLUTCH_H

#include <Arduino.h>
#include "Config.h"

class TouchClutch {
public:
  void begin();
  void update();
  
  bool isEngaged();           // True if thumb is currently touching (or override)
  bool isSettling();          // True during 50ms post-touch foam bounce
  bool isSprint();            // True if momentary 2x sprint is active (double-tap & hold)
  bool checkTripleTap();      // True once when triple-tap is detected
  int  getRawValue();
  int  getMinRaw() { return minRaw; }
  int  getMaxRaw() { return maxRaw; }
  void resetPeaks() { minRaw = lastRawValue; maxRaw = lastRawValue; }

private:
  int  lastRawValue        = 0;
  int  minRaw              = 999999;
  int  maxRaw              = 0;
  bool rawTouchedState     = false;
  bool prevTouchedState    = false;
  
  unsigned long touchDownTime    = 0;
  unsigned long lastReleaseTime  = 0;
  const unsigned long tapMaxDuration = 300; // ms: max hold duration to qualify as a tap
  const unsigned long tapGapTimeout  = 350; // ms: max gap between taps in a multi-tap sequence
  
  int  tapCount            = 0;
  bool isSprintActive      = false;
  bool tripleTapFired      = false;
  bool settlingActive      = false;
};

extern TouchClutch Clutch;

#endif // TOUCH_CLUTCH_H
