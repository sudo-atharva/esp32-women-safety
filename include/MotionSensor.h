#pragma once
#include <Arduino.h>

// Sudden-movement / impact detection off the MPU6050 accelerometer.
// Simple magnitude-over-threshold check -- not full fall-detection
// (free-fall + impact pattern); add that later if false positives/negatives
// on the simple threshold turn out to matter.
namespace MotionSensor {
  bool begin();
  bool checkSuddenMovement(); // call every loop(); returns true once per spike
}
