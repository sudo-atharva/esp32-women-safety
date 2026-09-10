#pragma once
#include <Arduino.h>

struct StatusInfo {
  bool gpsFix;
  bool gsmReady;
  int signalQuality; // -1 unknown
  String wearerName;
  String lastAction;  // e.g. "SOS sent", "Sending...", "Idle"
};

namespace Display {
  bool begin();               // returns false if OLED not found on the bus
  void show(const StatusInfo &s);
  void showMessage(const String &line1, const String &line2 = "");
}
