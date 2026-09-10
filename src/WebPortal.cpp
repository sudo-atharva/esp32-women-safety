#include "WebPortal.h"
#include "Config.h"
#include <WiFi.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>

namespace WebPortal {
  static AsyncWebServer server(80);

  static void handleConfigPost(AsyncWebServerRequest *request, uint8_t *data,
                                size_t len, size_t index, size_t total) {
    if (index == 0) request->_tempObject = new String();
    String *body = (String *)request->_tempObject;
    body->concat((const char *)data, len);

    if (index + len == total) {
      bool ok = Config::fromJson(*body);
      request->send(ok ? 200 : 400, "application/json",
                     ok ? "{\"ok\":true}" : "{\"ok\":false}");
      delete body;
      request->_tempObject = nullptr;
    }
  }

  void begin() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP("SafetyBand-Setup", "safety123");
    Serial.print("[WebPortal] AP IP: ");
    Serial.println(WiFi.softAPIP());

    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

    server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(200, "application/json", Config::toJson());
    });

    server.on(
        "/api/config", HTTP_POST, [](AsyncWebServerRequest *request) {},
        nullptr, handleConfigPost);

    server.onNotFound([](AsyncWebServerRequest *request) {
      request->send(404, "text/plain", "Not found");
    });

    server.begin();
  }
}
