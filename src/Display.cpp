#include "Display.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

namespace Display {
  static Adafruit_SSD1306 oled(128, 64, &Wire, -1);
  static bool ok = false;

  bool begin() {
    ok = oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    if (ok) {
      oled.clearDisplay();
      oled.setTextColor(SSD1306_WHITE);
      oled.setTextSize(1);
      oled.display();
    }
    return ok;
  }

  void show(const StatusInfo &s) {
    if (!ok) return;
    oled.clearDisplay();
    oled.setCursor(0, 0);
    oled.setTextSize(1);

    oled.println(s.wearerName);
    oled.println("--------------------");

    oled.print("GPS : ");
    oled.println(s.gpsFix ? "FIX" : "NO FIX");

    oled.print("GSM : ");
    if (!s.gsmReady) {
      oled.println("NO SIGNAL");
    } else if (s.signalQuality >= 0) {
      oled.print("OK ("); oled.print(s.signalQuality); oled.println(")");
    } else {
      oled.println("OK");
    }

    oled.println();
    oled.println(s.lastAction);

    oled.display();
  }

  void showMessage(const String &line1, const String &line2) {
    if (!ok) return;
    oled.clearDisplay();
    oled.setCursor(0, 0);
    oled.setTextSize(1);
    oled.println(line1);
    if (line2.length()) oled.println(line2);
    oled.display();
  }
}
