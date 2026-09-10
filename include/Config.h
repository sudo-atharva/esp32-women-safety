#pragma once
#include <Arduino.h>
#include <vector>

struct Geofence {
  bool enabled = false;
  double lat = 0.0;
  double lon = 0.0;
  float radiusM = 500.0f;
};

// Wearer profile + alert numbers + geofence, persisted as JSON on LittleFS.
// Edited locally over serial or remotely via the device's own WebPortal.
namespace Config {
  extern String name;
  extern std::vector<String> numbers;
  extern Geofence geofence;

  void begin();          // mounts LittleFS, loads config.json (writes defaults if missing)
  bool save();            // writes current in-memory values back to config.json
  String toJson();        // serialize current config (for the web UI)
  bool fromJson(const String &json); // apply + save a new config from the web UI

  // Great-circle distance in meters between (lat1,lon1) and (lat2,lon2).
  double distanceMeters(double lat1, double lon1, double lat2, double lon2);
}
