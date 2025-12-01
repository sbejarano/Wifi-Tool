#include <WiFi.h>
#include <TFT_eSPI.h>
#include <vector>
#include <algorithm>

TFT_eSPI tft = TFT_eSPI();

// =======================
// AP RECORD STRUCT
// =======================
struct APRecord {
  String ssid;
  String bssid;
  int rssi;
  int channel;
  int freq;
};

// =======================
// GLOBALS
// =======================
std::vector<APRecord> aps;
std::vector<APRecord> lastAps;

TaskHandle_t wifiTaskHandle;
TaskHandle_t displayTaskHandle;

bool newScanReady = false;

// Convert WiFi channel → frequency MHz
int chanToFreq(int ch) {
  return 2407 + (ch * 5);
}

// =======================
// RSSI COLOR FUNCTION
// =======================
uint16_t rssiColor(int rssi) {
  if (rssi >= -67)   return TFT_GREEN;          // Excellent–Good
  if (rssi >= -70)   return TFT_YELLOW;         // Fair
  if (rssi >= -80)   return TFT_RED;            // Poor
  if (rssi >= -90)   return 0x7800;             // Burgundy (RGB565)
  return TFT_BLACK;                              // Dead
}

// =======================
// WIFI SCAN TASK (CORE 0)
// =======================
void wifiScanTask(void *param) {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  for (;;) {
    int count = WiFi.scanNetworks(false, true);

    aps.clear();
    aps.reserve(count);

    for (int i = 0; i < count; i++) {

      // ===== Skip Hidden SSIDs =====
      String name = WiFi.SSID(i);
      if (name.length() == 0) {
        continue;
      }

      APRecord ap;
      ap.ssid    = name;
      ap.bssid   = WiFi.BSSIDstr(i);
      ap.rssi    = WiFi.RSSI(i);
      ap.channel = WiFi.channel(i);
      ap.freq    = chanToFreq(ap.channel);

      aps.push_back(ap);
    }

    // Sort strongest → weakest
    std::sort(aps.begin(), aps.end(),
      [](const APRecord &a, const APRecord &b) {
        return a.rssi > b.rssi;
      });

    // ===== SERIAL OUTPUT =====
    Serial.println("\n===== WiFi Scan (Visible SSIDs Only) =====");
    for (auto &ap : aps) {
      Serial.printf(
        "%s  %ddBm  CH%d  %dMHz  %s\n",
        ap.bssid.c_str(),
        ap.rssi,
        ap.channel,
        ap.freq,
        ap.ssid.c_str()
      );
    }

    newScanReady = true;
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}

// =======================
// DRAW ONE CARD (COLOR)
// =======================
void drawCard(const APRecord &ap, int y) {
  tft.fillRect(0, y, 320, 52, TFT_BLACK);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(4, y + 4);
  tft.printf("SSID: %s", ap.ssid.c_str());

  tft.setCursor(4, y + 20);
  tft.printf("BSSID: %s", ap.bssid.c_str());

  // ===== RSSI BAR COLOR =====
  uint16_t col = rssiColor(ap.rssi);

  int bar = map(ap.rssi, -90, -20, 10, 300);
  bar = constrain(bar, 10, 300);

  tft.fillRect(4, y + 38, bar, 8, col);
  tft.drawRect(4, y + 38, 300, 8, TFT_WHITE);

  // Right side text using same color
  tft.setTextColor(col, TFT_BLACK);
  tft.setCursor(240, y + 4);
  tft.printf("%ddBm", ap.rssi);

  tft.setCursor(240, y + 20);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.printf("CH%d", ap.channel);
}

// =======================
// DISPLAY TASK (CORE 1)
// Smooth Diff-Based Update
// =======================
void displayTask(void *param) {
  const int cardHeight = 52;
  const int maxCards   = 8;

  for (;;) {
    if (newScanReady) {
      newScanReady = false;

      for (int i = 0; i < maxCards; i++) {
        bool needRedraw = false;

        if (i >= aps.size()) {
          if (i < lastAps.size()) needRedraw = true;
        }
        else if (i >= lastAps.size()) {
          needRedraw = true;
        }
        else {
          if (aps[i].ssid    != lastAps[i].ssid)    needRedraw = true;
          if (aps[i].bssid   != lastAps[i].bssid)   needRedraw = true;
          if (aps[i].rssi    != lastAps[i].rssi)    needRedraw = true;
          if (aps[i].channel != lastAps[i].channel) needRedraw = true;
        }

        if (needRedraw) {
          int y = i * cardHeight;

          if (i < aps.size()) {
            drawCard(aps[i], y);
          } else {
            tft.fillRect(0, y, 320, cardHeight, TFT_BLACK);
          }
        }
      }

      lastAps = aps;
    }

    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

// =======================
// SETUP
// =======================
void setup() {
  Serial.begin(115200);

  // Backlight ON
  pinMode(27, OUTPUT);
  digitalWrite(27, HIGH);

  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(1);

  xTaskCreatePinnedToCore(wifiScanTask, "wifiTask", 6000, NULL, 1, &wifiTaskHandle, 0);
  xTaskCreatePinnedToCore(displayTask, "displayTask", 5000, NULL, 1, &displayTaskHandle, 1);
}

// =======================
// LOOP (unused)
// =======================
void loop() {
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}
