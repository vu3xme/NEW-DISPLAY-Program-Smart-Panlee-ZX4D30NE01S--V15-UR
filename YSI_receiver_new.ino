#include <Arduino_GFX_Library.h>
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "FreeMono8pt7b.h"
#include "FreeSansBold10pt7b.h"
#include "FreeSerifBoldItalic12pt7b.h"

#define GFX_BL DF_GFX_BL // default backlight pin, you may replace DF_GFX_BL to actual backlight pin
#define TFT_BL 45

/* More dev device declaration: https://github.com/moononournation/Arduino_GFX/wiki/Dev-Device-Declaration */

#if defined(DISPLAY_DEV_KIT)
Arduino_GFX *gfx = create_default_Arduino_GFX();
#else /* !defined(DISPLAY_DEV_KIT) */
Arduino_ESP32RGBPanel *bus = new Arduino_ESP32RGBPanel(
    GFX_NOT_DEFINED /* CS */, GFX_NOT_DEFINED /* SCK */, GFX_NOT_DEFINED /* SDA */,
    40 /* DE */, 48 /* VSYNC */, 47 /* HSYNC */, 14 /* PCLK */,
    4 /* R0 */, 5 /* R1 */, 6 /* R2 */, 7 /* R3 */, 15 /* R4 */,
    16 /* G0 */, 17 /* G1 */, 18 /* G2 */, 8 /* G3 */, 3 /* G4 */, 46 /* G5 */,
    9 /* B0 */, 10 /* B1 */, 11 /* B2 */, 12 /* B3 */, 13 /* B4 */
);
// option 1:
// ILI6485 LCD 480x272
Arduino_RPi_DPI_RGBPanel *gfx = new Arduino_RPi_DPI_RGBPanel(
  bus,
  480 /* width */, 0 /* hsync_polarity */, 8 /* hsync_front_porch */, 4 /* hsync_pulse_width */, 43 /* hsync_back_porch */,
  272 /* height */, 0 /* vsync_polarity */, 8 /* vsync_front_porch */, 4 /* vsync_pulse_width */, 12 /* vsync_back_porch */,
  1 /* pclk_active_neg */, 9000000 /* prefer_speed */, true /* auto_flush */);
#endif /* !defined(DISPLAY_DEV_KIT) */

/*******************************************************************************
 * End of Arduino_GFX setting
 ******************************************************************************/

const char* ssid = "YSI_AP";
const char* password = "12345678";

float doMgValue = 0;
float temp = 0;
bool aerationOn = false;

void setup(void) {
  Serial.begin(9600);
  gfx->begin();
  gfx->fillScreen(BLACK);
#ifdef TFT_BL
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
#endif
  gfx->setCursor(6, 40);
  gfx->setFont(&FreeSansBold10pt7b);
  gfx->setTextColor(GREEN);
  gfx->println("ICAR-CIFRI NePPA Project funded by ICAR!");
  gfx->println("Dissolved Oxygen Sensor>");
  delay(5000);
  if (connectToWiFi()) {
    fetchData();
  }
}

void loop() {
  fetchData();
  updateDisplay();
  delay(5000); // Fetch and update every 5 seconds
}

bool connectToWiFi() {
  int retryCount = 0;
  const int maxRetryCount = 10;
  while (retryCount < maxRetryCount) {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.println("Connecting to WiFi...");
      gfx->setFont(&FreeSansBold10pt7b);
      gfx->setTextColor(WHITE);
      gfx->println("Sensor Connecting!!!!!");
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Connected to WiFi");
      gfx->setFont(&FreeSerifBoldItalic12pt7b);
      gfx->setTextColor(WHITE);
      gfx->println("Sensor Connected");
      return true;
    }
    retryCount++;
  }
  Serial.println("Failed to connect to WiFi");
  gfx->setFont(&FreeSerifBoldItalic12pt7b);
  gfx->setTextColor(RED);
  gfx->println("Failed to connect to WiFi");
  return false;
}

void fetchData() {
  HTTPClient http;
  http.begin("http://192.168.4.1/data");
  int httpCode = http.GET();
  if (httpCode == 200) {
    String payload = http.getString();
    // Parse sensor data
    doMgValue = atof(getValue(payload, "Dissolved_Oxygen_(DO)(mg/L): ").c_str());
    temp = atof(getValue(payload, "temparature(C): ").c_str());
    aerationOn = getValueAsInt(payload, "Aerator_status: "); // Changed to int
  } else {
    Serial.println("Error fetching data");
  }
  http.end();
}

String getValue(String data, String key) {
  int startIndex = data.indexOf(key) + key.length();
  int endIndex = data.indexOf('\n', startIndex);
  return data.substring(startIndex, endIndex);
}

int getValueAsInt(String data, String key) { // New function for converting to int
  int startIndex = data.indexOf(key) + key.length();
  int endIndex = data.indexOf('\n', startIndex);
  String valueStr = data.substring(startIndex, endIndex);
  return valueStr.toInt();
}

void updateDisplay() {
  gfx->fillScreen(BLACK);
  gfx->setFont(&FreeSansBold10pt7b);
  gfx->setTextColor(GREENYELLOW);
  gfx->setCursor(100, 20);
  gfx->println("Precision Agriculture (NePPA)");
  gfx->setCursor(60, 50);
  gfx->println("ICAR-CIFRI DO Monitoring System (V1.0.1)");
  // Update DO Gauge
  drawGauge(15, 60, 170, doMgValue, 0, 20, NAVY, ORANGE, GREEN, DARKGREEN, RED, "Do(mg/L)");
  // Update Temperature Gauge
  drawGauge(260, 60, 170, temp, 0, 50, ORANGE, PINK, GREEN, GREEN, RED, "Temp(C)");
  // Print Aerator Status
  gfx->setFont(&FreeSerifBoldItalic12pt7b);
  gfx->setTextColor(OLIVE);
  gfx->setCursor(10, 250);
  gfx->println("Aerator: " + String((aerationOn == 1 ? "On" : "Off"))); // Simplified
  // Print WiFi Status
  gfx->setFont(&FreeSerifBoldItalic12pt7b);
  gfx->setTextColor(GREENYELLOW);
  gfx->setCursor(210, 250);
  gfx->println("WiFi: " + WiFi.SSID() + " (" + String(WiFi.RSSI()) + "dBm)");
}

void drawGauge(int x, int y, int size, float currentValue, float minValue, float maxValue,
               unsigned int color1, unsigned int color2, unsigned int color3,
               unsigned int color4, unsigned int color5, String label) {
  // Calculate angles for minimum, current, and maximum values
  float angleMin = map(minValue, minValue, maxValue, 30, 330);
  float angleMax = map(maxValue, minValue, maxValue, 30, 330);
  float angleValue = map(currentValue, minValue, maxValue, angleMin, angleMax);
  // Define the thickness of the arcs and the needle
  int arcThickness = 10; // Adjust this value as needed
  int needleLength = size / 2 - arcThickness - 10; // Length of the needle
  int needleWidth = 8; // Width of the needle
  // Draw background arc
  for (int i = 0; i < arcThickness; i++) {
    gfx->drawArc(x + size / 2, y + size / 2, size / 2 - i, size / 2 - i - 1, 30, 330, color1);
  }
  // Draw minimum value arc
  for (int i = 0; i < arcThickness; i++) {
    gfx->drawArc(x + size / 2, y + size / 2, size / 2 - i - 4, size / 2 - i - 5, 30, angleMin, color2);
  }
  // Draw current value arc
  for (int i = 0; i < arcThickness; i++) {
    gfx->drawArc(x + size / 2, y + size / 2, size / 2 - i - 4, size / 2 - i - 5, 30, angleValue, color4);
  }
  // Draw maximum value arc
  for (int i = 0; i < arcThickness; i++) {
    gfx->drawArc(x + size / 2, y + size / 2, size / 2 - i - 4, size / 2 - i - 5, angleMax, 330, color3);
  }
  // Draw value text
  gfx->setFont(&FreeSansBold10pt7b);
  gfx->setTextColor(WHITE);
  gfx->setCursor(x + size / 2 - 30, y + size / 2 + 10);
  gfx->println(label + ": " + String(currentValue, 2));
  // Draw minimum value text inside the arc
  int minTextX = x + size / 2 - 30 + (size / 2 - arcThickness - 4) * cos(radians(angleMin));
  int minTextY = y + size / 2 + 10 + (size / 2 - arcThickness - 4) * sin(radians(angleMin));
  gfx->setTextColor(color2);
  gfx->setCursor(minTextX, minTextY);
  gfx->println(String(minValue, 2));
  // Draw maximum value text inside the arc
  int maxTextX = x + size / 2 - 30 + (size / 2 - arcThickness - 4) * cos(radians(angleMax));
  int maxTextY = y + size / 2 + 10 + (size / 2 - arcThickness - 4) * sin(radians(angleMax));
  gfx->setTextColor(color3);
  gfx->setCursor(maxTextX, maxTextY);
  gfx->println(String(maxValue, 2));
  // Draw the needle with a front arrow
  float needleAngle = radians(angleValue);
  int needleFrontX = x + size / 2 + (needleLength - 10) * cos(needleAngle);
  int needleFrontY = y + size / 2 + (needleLength - 10) * sin(needleAngle);
  int needleBackX = x + size / 2 - (needleWidth / 2) * cos(needleAngle + PI / 2);
  int needleBackY = y + size / 2 - (needleWidth / 2) * sin(needleAngle + PI / 2);
  int needleBackX2 = x + size / 2 + (needleWidth / 2) * cos(needleAngle + PI / 2);
  int needleBackY2 = y + size / 2 + (needleWidth / 2) * sin(needleAngle + PI / 2);
  gfx->fillTriangle(needleFrontX, needleFrontY, needleBackX, needleBackY, needleBackX2, needleBackY2, color5);
}