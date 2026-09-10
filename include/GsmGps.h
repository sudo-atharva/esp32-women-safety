#pragma once
#include <Arduino.h>

// Thin AT-command wrapper around a Quectel EC200EU for SMS + GNSS.
// Blocking on purpose -- SMS/GPS fixes are rare, low-frequency events here,
// simplest correct thing beats an async AT state machine for this project.
namespace GsmGps {
  void begin(HardwareSerial &serial, int rxPin, int txPin, int pwrKeyPin);

  bool isNetworkRegistered();  // AT+CREG? -> registered home/roaming
  int  signalQuality();        // AT+CSQ -> 0-31, -1 if unknown/no signal (99)

  // Turns on GNSS if needed and reads a fix via AT+QGPSLOC.
  // Returns false if no fix yet (keeps GNSS engine running for next try).
  bool getLocation(double &lat, double &lon);

  bool sendSms(const String &number, const String &message);
}
