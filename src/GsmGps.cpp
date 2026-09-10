#include "GsmGps.h"

namespace GsmGps {
  static HardwareSerial *_serial = nullptr;
  static int _pwrKeyPin = -1;

  // Sends one AT command, collects everything the module replies until it
  // sees OK/ERROR or the timeout expires.
  static String sendAT(const String &cmd, unsigned long timeoutMs = 2000) {
    while (_serial->available()) _serial->read();
    Serial.print("[GSM TX] ");
    Serial.println(cmd);
    _serial->print(cmd);
    _serial->print("\r\n");

    String resp;
    unsigned long start = millis();
    while (millis() - start < timeoutMs) {
      while (_serial->available()) {
        resp += (char)_serial->read();
      }
      if (resp.indexOf("OK\r\n") != -1 || resp.indexOf("ERROR") != -1) break;
    }
    Serial.print("[GSM RX] ");
    Serial.println(resp.length() ? resp : "(timeout, no response)");
    return resp;
  }

  static void powerOnPulse() {
    if (_pwrKeyPin < 0) return;
    // EC200EU: hold PWRKEY low ~600ms to boot the module.
    // ponytail: fixed pulse timing, adjust to your exact board/module rev if it won't boot.
    digitalWrite(_pwrKeyPin, LOW);
    delay(600);
    digitalWrite(_pwrKeyPin, HIGH);
  }

  void begin(HardwareSerial &serial, int rxPin, int txPin, int pwrKeyPin) {
    _serial = &serial;
    _pwrKeyPin = pwrKeyPin;

    if (_pwrKeyPin >= 0) {
      pinMode(_pwrKeyPin, OUTPUT);
      digitalWrite(_pwrKeyPin, HIGH);
    }

    _serial->begin(115200, SERIAL_8N1, rxPin, txPin);
    delay(200);
    Serial.println("[GSM] pulsing PWRKEY...");
    powerOnPulse();

    // Listen raw during boot window — module usually sends an unsolicited
    // banner (e.g. "RDY") here even before any AT is sent. If nothing shows
    // up at all, it's a wiring/power/level issue, not a command issue.
    Serial.println("[GSM] listening for boot banner...");
    unsigned long bootStart = millis();
    String boot;
    while (millis() - bootStart < 3000) {
      while (_serial->available()) boot += (char)_serial->read();
    }
    Serial.print("[GSM RAW BOOT] ");
    Serial.println(boot.length() ? boot : "(nothing received on RX at all)");

    // Wait for the module to respond to plain AT (up to ~10s).
    Serial.println("[GSM] waiting for module...");
    bool gotAT = false;
    for (int i = 0; i < 10; i++) {
      if (sendAT("AT", 1000).indexOf("OK") != -1) { gotAT = true; break; }
      delay(500);
    }
    Serial.println(gotAT ? "[GSM] module responding" : "[GSM] module NOT responding after 10 tries");

    sendAT("ATE0");              // echo off
    sendAT("AT+CMGF=1");         // SMS text mode
    sendAT("AT+CSCS=\"GSM\"");   // GSM 7-bit charset
    sendAT("AT+CNMI=0,0,0,0,0"); // no unsolicited SMS notifications
  }

  bool isNetworkRegistered() {
    String r = sendAT("AT+CREG?");
    int idx = r.indexOf("+CREG:");
    if (idx == -1) return false;
    int comma = r.indexOf(',', idx);
    if (comma == -1) return false;
    int stat = r.substring(comma + 1).toInt();
    return stat == 1 || stat == 5; // registered, home or roaming
  }

  int signalQuality() {
    String r = sendAT("AT+CSQ");
    int idx = r.indexOf("+CSQ:");
    if (idx == -1) return -1;
    int rssi = r.substring(idx + 5).toInt();
    if (rssi == 99) return -1; // unknown
    return rssi;
  }

  bool getLocation(double &lat, double &lon) {
    String state = sendAT("AT+QGPS?", 1000);
    if (state.indexOf("+QGPS: 1") == -1) {
      sendAT("AT+QGPS=1", 2000); // power on GNSS engine, fix takes time
    }

    String r = sendAT("AT+QGPSLOC=2", 2000);
    int idx = r.indexOf("+QGPSLOC:");
    if (idx == -1) return false; // no fix yet

    String line = r.substring(idx + 9);
    line = line.substring(0, line.indexOf('\r'));

    // <UTC>,<lat>,<lon>,<hdop>,<alt>,<fix>,<cog>,<spkm>,<spkn>,<date>,<nsat>
    int c1 = line.indexOf(',');
    int c2 = line.indexOf(',', c1 + 1);
    int c3 = line.indexOf(',', c2 + 1);
    if (c1 == -1 || c2 == -1 || c3 == -1) return false;

    lat = line.substring(c1 + 1, c2).toDouble();
    lon = line.substring(c2 + 1, c3).toDouble();
    return lat != 0.0 || lon != 0.0;
  }

  bool sendSms(const String &number, const String &message) {
    Serial.println("[SMS] sending to " + number + ": " + message);
    sendAT("AT+CMGF=1", 1000);

    while (_serial->available()) _serial->read();
    String cmd = "AT+CMGS=\"" + number + "\"";
    Serial.print("[GSM TX] ");
    Serial.println(cmd);
    _serial->print(cmd);
    _serial->print("\r");

    // wait for the '>' prompt
    String resp;
    unsigned long start = millis();
    while (millis() - start < 3000) {
      while (_serial->available()) resp += (char)_serial->read();
      if (resp.indexOf('>') != -1) break;
    }
    Serial.print("[GSM RX] ");
    Serial.println(resp.length() ? resp : "(timeout, no prompt)");
    if (resp.indexOf('>') == -1) {
      Serial.println("[SMS] failed: no '>' prompt");
      return false;
    }

    _serial->print(message);
    _serial->write(0x1A); // Ctrl+Z sends the message

    resp = "";
    start = millis();
    while (millis() - start < 15000) {
      while (_serial->available()) resp += (char)_serial->read();
      if (resp.indexOf("+CMGS:") != -1 || resp.indexOf("ERROR") != -1) break;
    }
    Serial.print("[GSM RX] ");
    Serial.println(resp.length() ? resp : "(timeout, no response)");
    bool ok = resp.indexOf("+CMGS:") != -1;
    Serial.println(ok ? "[SMS] sent OK" : "[SMS] failed");
    return ok;
  }
}
