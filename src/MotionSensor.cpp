#include "MotionSensor.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <math.h>

namespace MotionSensor {
  static Adafruit_MPU6050 mpu;
  static bool armed = true;          // re-armed once magnitude falls back to normal
  static unsigned long lastTrigger = 0;

  static const float SPIKE_THRESHOLD_G = 2.5f;     // sudden jolt / fall impact
  static const float REARM_THRESHOLD_G = 1.5f;     // must settle below this to re-arm
  static const unsigned long TRIGGER_COOLDOWN_MS = 5000;

  bool begin() {
    if (!mpu.begin()) return false;
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    return true;
  }

  bool checkSuddenMovement() {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    // magnitude in g (accel values from the lib are in m/s^2)
    float magG = sqrtf(a.acceleration.x * a.acceleration.x +
                        a.acceleration.y * a.acceleration.y +
                        a.acceleration.z * a.acceleration.z) / 9.80665f;

    if (!armed) {
      if (magG < REARM_THRESHOLD_G) armed = true;
      return false;
    }

    unsigned long now = millis();
    if (magG >= SPIKE_THRESHOLD_G && now - lastTrigger > TRIGGER_COOLDOWN_MS) {
      lastTrigger = now;
      armed = false;
      return true;
    }
    return false;
  }
}
