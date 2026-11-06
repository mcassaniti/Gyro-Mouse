#include "MPUManager.h"

MPUManager Gyro;

void MPUManager::begin() {
  pinMode(PIN_I2C_SDA, INPUT_PULLUP);
  pinMode(PIN_I2C_SCL, INPUT_PULLUP);
  
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(100000);

  scanI2C();

  // Try 0x68 first, then 0x69
  mpu.setAddress(0x68);
  byte status = mpu.begin(0, 0); // gyro +-250 deg/s, accel +-2g

  if (status != 0) {
    mpu.setAddress(0x69);
    status = mpu.begin(0, 0);
  }

  if (status == 0) {
    ready = true;
    Serial.printf("[OK]   MPU-6050 initialized successfully at I2C address 0x%02X!\n", mpu.getAddress());
  } else {
    ready = false;
    Serial.printf("[ERROR] MPU-6050 begin() failed with status code: %d\n", status);
  }
}

bool MPUManager::isReady() {
  return ready;
}

void MPUManager::update() {
  if (!ready) return;
  mpu.update();

  // Left edge of chip is Top/Front of mouse:
  // Forward/Back Pitch is on -ax, Left/Right Roll is on -ay
  float ax = mpu.getAccX();
  float ay = mpu.getAccY();
  float az = mpu.getAccZ();

  rawPitch = atan2(-ax, az) * 180.0f / PI;
  rawRoll  = atan2(-ay, az) * 180.0f / PI;
}

void MPUManager::calibrateTare(const char* reason) {
  if (!ready) return;

  Serial.printf("\n[TARE] Starting Tare Calibration [%s]...\n", reason);
  Serial.println(F("[TARE] Averaging 10 resting samples over 500ms..."));

  // LED on during calibration (Amber/Orange while sampling)
  setStatusLED(true, 64, 32, 0);

  float sumP = 0.0f;
  float sumR = 0.0f;
  const int samples = 10;

  for (int i = 0; i < samples; i++) {
    mpu.update();
    float ax = mpu.getAccX();
    float ay = mpu.getAccY();
    float az = mpu.getAccZ();

    sumP += atan2(-ax, az) * 180.0f / PI;
    sumR += atan2(-ay, az) * 180.0f / PI;
    delay(50);
  }

  Config.config.pitchOffset = sumP / samples;
  Config.config.rollOffset  = sumR / samples;

  Serial.printf("[TARE] Complete (Dynamic RAM) -> Pitch Offset: %+6.2f deg | Roll Offset: %+6.2f deg\n\n",
                Config.config.pitchOffset, Config.config.rollOffset);

  setStatusLED(false);
}

float MPUManager::getRawPitch() {
  return rawPitch;
}

float MPUManager::getRawRoll() {
  return rawRoll;
}

float MPUManager::getActivePitch() {
  return rawPitch - Config.config.pitchOffset;
}

float MPUManager::getActiveRoll() {
  return rawRoll - Config.config.rollOffset;
}

float MPUManager::getAccX() { return mpu.getAccX(); }
float MPUManager::getAccY() { return mpu.getAccY(); }
float MPUManager::getAccZ() { return mpu.getAccZ(); }
float MPUManager::getTemp() { return mpu.getTemp(); }

void MPUManager::scanI2C() {
  Serial.println(F("[I2C SCAN] Scanning I2C bus on GPIO 4 (SDA) & GPIO 5 (SCL)..."));
  int count = 0;
  for (uint8_t address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      Serial.printf("[I2C SCAN] -> Device found at 0x%02X", address);
      if (address == 0x68) Serial.print(F(" (MPU Default)"));
      else if (address == 0x69) Serial.print(F(" (MPU AD0 High)"));
      Serial.println();
      count++;
    }
  }
  if (count == 0) {
    Serial.println(F("[I2C SCAN] -> No I2C devices found! Check wiring and power."));
  }
}
