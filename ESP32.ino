#include <WiFi.h>
#include <TFT_eSPI.h>
#include <vector>
#include <algorithm>

extern "C" {
  #include "esp_wifi.h"
}

TFT_eSPI tft = TFT_eSPI();

// =======================
// LED PIN (single red, active-LOW)
// =======================
#define LED_PIN 4    // LOW = ON, HIGH = OFF

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

// =======================
// HELPERS
// =======================
int chanToFreq(int ch) {
  return 2407 + (ch * 5);
}

// Convert MAC to string
String macToString(const uint8_t *mac) {
  char buf[18];
  sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X",
          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(buf);
}

// RSSI → color
uint16_t rssiColor(int rssi) {
  if (rssi >= -67) return TFT_GREEN;   // Excellent/Good
  if (rssi >= -70) return TFT_YELLOW;  // Fair
  if (rssi >= -80) return TFT_RED;     // Poor
  return 0x7800;                       // Very poor (burgundy)
}

// ===============================
// FAST WIFI SCAN TASK (CORE 0)
// ===============================
void wifiScanTask(void *param) {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Configure fast active scan
  wifi_scan_config_t cfg;
  memset(&cfg, 0, sizeof(cfg));
  cfg.scan_type = WIFI_SCAN_TYPE_ACTIVE;
  cfg.show_hidden = true;              // we filter hidden later
  cfg.scan_time.active.min = 40;       // ~40 ms per channel
  cfg.scan_time.active.max = 60;       // ~60 ms per channel

  for (;;) {

    // LED OFF during scan (active-LOW)
    digitalWrite(LED_PIN, HIGH);

    // Start scan (blocking until done)
    esp_wifi_scan_start(&cfg, true);

    // Get AP count
    uint16_t count = 0;
    esp_wifi_scan_get_ap_num(&count);

    wifi_ap_record_t *list = nullptr;
    if (count > 0) {
      list = (wifi_ap_record_t *)malloc(count * sizeof(wifi_ap_record_t));
    }

    if (list && count > 0) {
      esp_wifi_scan_get_ap_records(&count, list);

      aps.clear();
      aps.reserve(count);

      for (int i = 0; i < count; i++) {
        // Skip hidden SSID
        if (strlen((char*)list[i].ssid) == 0) continue;

        APRecord ap;
        ap.ssid    = String((char*)list[i].ssid);
        ap.bssid   = macToString(list[i].bssid);
        ap.rssi    = list[i].rssi;
        ap.channel = list[i].primary;
        ap.freq    = chanToFreq(ap.channel);

        aps.push_back(ap);
      }

      free(list);
    } else {
      aps.clear();
      if (list) free(list);
    }

    // Sort strongest first
    std::sort(aps.begin(), aps.end(),
      [](const APRecord &a, const APRecord &b) {
        return a.rssi > b.rssi;
      });

    // Serial output
    Serial.println("\n===== FAST WiFi Scan (target ~1s interval) =====");
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

    // Blink LED once to indicate scan complete (active-LOW)
    digitalWrite(LED_PIN, LOW);                 // ON
    vTaskDelay(60 / portTICK_PERIOD_MS);
    digitalWrite(LED_PIN, HIGH);                // OFF

    newScanReady = true;

    // Top-up delay to ~1s total loop time
    vTaskDelay(700 / portTICK_PERIOD_MS);
  }
}

// ===============================
// DRAW CARD
// ===============================
void drawCard(const APRecord &ap, int y) {
  tft.fillRect(0, y, 320, 52, TFT_BLACK);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(4, y + 4);
  tft.printf("SSID: %s", ap.ssid.c_str());

  tft.setCursor(4, y + 20);
  tft.printf("BSSID: %s", ap.bssid.c_str());

  uint16_t col = rssiColor(ap.rssi);
  int bar = map(ap.rssi, -90, -20, 10, 300);
  bar = constrain(bar, 10, 300);

  tft.fillRect(4, y + 38, bar, 8, col);
  tft.drawRect(4, y + 38, 300, 8, TFT_WHITE);

  tft.setTextColor(col, TFT_BLACK);
  tft.setCursor(240, y + 4);
  tft.printf("%ddBm", ap.rssi);

  tft.setCursor(240, y + 20);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.printf("CH%d", ap.channel);
}

// ===============================
// DISPLAY TASK (CORE 1)
// ===============================
void displayTask(void *param) {
  const int cardHeight = 52;
  const int maxCards   = 9;

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

// ===============================
// SETUP
// ===============================
void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);       // OFF (active-LOW)

  pinMode(27, OUTPUT);
  digitalWrite(27, HIGH);            // Backlight ON

  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(1);

  xTaskCreatePinnedToCore(wifiScanTask, "wifiTask", 6000, NULL, 1, &wifiTaskHandle, 0);
  xTaskCreatePinnedToCore(displayTask, "displayTask", 5000, NULL, 1, &displayTaskHandle, 1);
}

// ===============================
// LOOP
// ===============================
void loop() {
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}

