#ifndef MPU_MANAGER_H
#define MPU_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <MPU6050_light.h>
#include "Config.h"

class MPUManager {
public:
  void begin();
  bool isReady();
  void update();
  
  void calibrateTare(const char* reason = "Manual");
  
  float getRawPitch();
  float getRawRoll();
  float getActivePitch();
  float getActiveRoll();
  
  float getAccX();
  float getAccY();
  float getAccZ();
  float getTemp();

  void scanI2C();

private:
  MPU6050 mpu{Wire};
  bool ready = false;
  float rawPitch = 0.0f;
  float rawRoll  = 0.0f;
};

extern MPUManager Gyro;

#endif // MPU_MANAGER_H
