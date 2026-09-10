#include <Arduino.h>
#include <Wire.h>
#include "Config.h"
#include "GsmGps.h"
#include "Display.h"
#include "MotionSensor.h"
#include "WebPortal.h"

// ---- Pin map (adjust to your wiring) ----
#define GSM_RX_PIN 16   // ESP32 RX2 <- EC200EU TX
#define GSM_TX_PIN 17   // ESP32 TX2 -> EC200EU RX
#define GSM_PWRKEY_PIN 4
#define BTN_EMERGENCY_PIN 32
#define BTN_LOCATION_PIN 33

#define STATUS_REFRESH_MS 15000UL
#define DEBOUNCE_MS 250UL

static bool gpsFix = false;
static double lastLat = 0, lastLon = 0;
static bool gsmReady = false;
static int signalQ = -1;
static bool geofenceInside = true;

static unsigned long lastStatusRefresh = 0;
static unsigned long lastEmergencyPress = 0;
static unsigned long lastLocationPress = 0;

static String buildMapsLink(double lat, double lon) {
  return "https://maps.google.com/?q=" + String(lat, 6) + "," + String(lon, 6);
}

static void refreshStatus() {
  StatusInfo s;
  s.gpsFix = gpsFix;
  s.gsmReady = gsmReady;
  s.signalQuality = signalQ;
  s.wearerName = Config::name;
  s.lastAction = "Idle";
  Display::show(s);
}

static void triggerSos(const String &reason) {
  Serial.println("[SOS] triggered: " + reason);
  Display::showMessage("SOS triggering...", reason);

  gpsFix = GsmGps::getLocation(lastLat, lastLon);
  String link = gpsFix ? buildMapsLink(lastLat, lastLon) : "location unavailable";
  String msg = "SOS! " + Config::name + " needs help.\nReason: " + reason + "\n" + link;

  for (auto &number : Config::numbers) {
    GsmGps::sendSms(number, msg);
  }

  Display::showMessage("SOS sent", reason);
  lastStatusRefresh = 0; // force a status redraw next loop
}

static void sendLocationUpdate() {
  Serial.println("[LOCATION] update requested");
  Display::showMessage("Sending location...");

  gpsFix = GsmGps::getLocation(lastLat, lastLon);
  String link = gpsFix ? buildMapsLink(lastLat, lastLon) : "location unavailable";
  String msg = Config::name + " - current location:\n" + link;

  for (auto &number : Config::numbers) {
    GsmGps::sendSms(number, msg);
  }

  Display::showMessage("Location sent");
  lastStatusRefresh = 0;
}

static void checkGeofence() {
  if (!Config::geofence.enabled || !gpsFix) return;

  double d = Config::distanceMeters(lastLat, lastLon, Config::geofence.lat, Config::geofence.lon);
  bool inside = d <= Config::geofence.radiusM;

  if (!inside && geofenceInside) {
    triggerSos("Left safe zone");
  } else if (inside && !geofenceInside) {
    Serial.println("[GEOFENCE] back inside");
  }
  geofenceInside = inside;
}

// active-low button with simple time-based debounce, returns true once per press
static bool pressed(int pin, unsigned long &lastPressTime) {
  if (digitalRead(pin) != LOW) return false;
  unsigned long now = millis();
  if (now - lastPressTime < DEBOUNCE_MS) return false;
  lastPressTime = now;
  return true;
}

void setup() {
  Serial.begin(115200);

  pinMode(BTN_EMERGENCY_PIN, INPUT_PULLUP);
  pinMode(BTN_LOCATION_PIN, INPUT_PULLUP);

  Wire.begin(); // shared I2C bus: OLED (0x3C) + MPU6050 (0x68)

  Config::begin();

  if (!Display::begin()) {
    Serial.println("[main] OLED not found");
  }
  Display::showMessage("Booting...", "Starting GSM/GPS");

  if (!MotionSensor::begin()) {
    Serial.println("[main] MPU6050 not found");
  }

  GsmGps::begin(Serial2, GSM_RX_PIN, GSM_TX_PIN, GSM_PWRKEY_PIN);

  WebPortal::begin();

  refreshStatus();
}

void loop() {
  if (MotionSensor::checkSuddenMovement()) {
    triggerSos("Sudden movement detected");
  }

  if (pressed(BTN_EMERGENCY_PIN, lastEmergencyPress)) {
    Serial.println("[BUTTON] emergency pressed");
    triggerSos("Panic button pressed");
  }

  if (pressed(BTN_LOCATION_PIN, lastLocationPress)) {
    Serial.println("[BUTTON] location pressed");
    sendLocationUpdate();
  }

  unsigned long now = millis();
  if (now - lastStatusRefresh >= STATUS_REFRESH_MS) {
    lastStatusRefresh = now;

    Serial.println("[STATUS] refreshing...");
    gsmReady = GsmGps::isNetworkRegistered();
    signalQ = GsmGps::signalQuality();
    gpsFix = GsmGps::getLocation(lastLat, lastLon);
    Serial.printf("[STATUS] gsm=%d signal=%d gpsFix=%d lat=%f lon=%f\n",
                   gsmReady, signalQ, gpsFix, lastLat, lastLon);

    checkGeofence();
    refreshStatus();
  }
}
