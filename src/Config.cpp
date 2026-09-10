#include "Config.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <math.h>

namespace Config {
  String name = "Unknown";
  std::vector<String> numbers;
  Geofence geofence;

  static const char *CONFIG_PATH = "/config.json";

  static void loadDefaults() {
    name = "Unknown";
    numbers.clear();
    numbers.push_back("+10000000000");
    geofence = Geofence();
  }

  bool fromJson(const String &json) {
    JsonDocument doc;
    if (deserializeJson(doc, json) != DeserializationError::Ok) return false;

    name = doc["name"] | "Unknown";

    numbers.clear();
    if (doc["numbers"].is<JsonArray>()) {
      for (JsonVariant v : doc["numbers"].as<JsonArray>()) {
        String n = v.as<String>();
        if (n.length()) numbers.push_back(n);
      }
    }
    if (numbers.empty()) numbers.push_back("+10000000000");

    JsonObject gf = doc["geofence"];
    geofence.enabled = gf["enabled"] | false;
    geofence.lat = gf["lat"] | 0.0;
    geofence.lon = gf["lon"] | 0.0;
    geofence.radiusM = gf["radiusM"] | 500.0f;

    return save();
  }

  String toJson() {
    JsonDocument doc;
    doc["name"] = name;
    JsonArray arr = doc["numbers"].to<JsonArray>();
    for (auto &n : numbers) arr.add(n);
    JsonObject gf = doc["geofence"].to<JsonObject>();
    gf["enabled"] = geofence.enabled;
    gf["lat"] = geofence.lat;
    gf["lon"] = geofence.lon;
    gf["radiusM"] = geofence.radiusM;

    String out;
    serializeJson(doc, out);
    return out;
  }

  bool save() {
    File f = LittleFS.open(CONFIG_PATH, "w");
    if (!f) return false;
    String json = toJson();
    f.print(json);
    f.close();
    return true;
  }

  void begin() {
    if (!LittleFS.begin(true)) {
      Serial.println("[Config] LittleFS mount failed");
    }
    loadDefaults();

    if (LittleFS.exists(CONFIG_PATH)) {
      File f = LittleFS.open(CONFIG_PATH, "r");
      String json = f.readString();
      f.close();
      if (!fromJson(json)) {
        Serial.println("[Config] config.json invalid, using defaults");
        loadDefaults();
        save();
      }
    } else {
      save();
    }
  }

  double distanceMeters(double lat1, double lon1, double lat2, double lon2) {
    const double R = 6371000.0; // earth radius, meters
    double dLat = radians(lat2 - lat1);
    double dLon = radians(lon2 - lon1);
    double a = sin(dLat / 2) * sin(dLat / 2) +
               cos(radians(lat1)) * cos(radians(lat2)) *
               sin(dLon / 2) * sin(dLon / 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));
    return R * c;
  }
}
