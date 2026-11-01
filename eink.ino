// Hourly R2 picture frame for Waveshare ESP32-S3-ePaper-13.3E6.
// Input: headerless, 1200x1600, 4 bits/pixel, high nibble first.
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <esp_heap_caps.h>
#include <esp_sleep.h>
#include <driver/gpio.h>
#include <time.h>
#include "config.h"
#include "FrameConsole.h"
#include "RootCA.h"
#include "EPD_13in3e.h"

constexpr size_t FRAME_BYTES = 1200UL * 1600UL / 2;
constexpr uint32_t WIFI_TIMEOUT_MS = 25000;
constexpr uint32_t DOWNLOAD_TIMEOUT_MS = 75000;
static uint8_t *frame = nullptr;

static void powerOffPanel() {
  DEV_Module_Exit();
  // Low signal pins avoid driving an unpowered panel through its inputs.
  const int pins[] = {EPD_CS_M_PIN, EPD_CS_S_PIN, EPD_DC_PIN,
                      EPD_SCK_PIN, EPD_MOSI_PIN};
  for (int pin : pins) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }
}

static void sleepUntilNextHour() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  powerOffPanel();
  // Keep the panel's power gate and reset low throughout deep sleep.
  gpio_hold_en(static_cast<gpio_num_t>(EPD_PWR_PIN));
  gpio_hold_en(static_cast<gpio_num_t>(EPD_RST_PIN));
  gpio_deep_sleep_hold_en();
  if (frame) { free(frame); frame = nullptr; }
  // Subtract this wake's work: roughly one hour between wake-ups.
  const uint64_t periodUs = REFRESH_SECONDS * 1000000ULL;
  const uint64_t elapsedUs = static_cast<uint64_t>(millis()) * 1000ULL;
  const uint64_t sleepUs = elapsedUs < periodUs ? periodUs - elapsedUs : periodUs;
  FrameLog.printf("Sleeping for %llu seconds\n", sleepUs / 1000000ULL);
  esp_sleep_enable_timer_wakeup(sleepUs);
  FrameLog.flush();
  esp_deep_sleep_start();
}

static bool connectWiFi() {
  if (!strcmp(WIFI_SSID, "YOUR_WIFI_NAME")) {
    FrameLog.println("Set WIFI_SSID and WIFI_PASSWORD in config.h first.");
    return false;
  }
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  FrameLog.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  const uint32_t started = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - started >= WIFI_TIMEOUT_MS) {
      FrameLog.println("Wi-Fi timeout; keeping existing picture.");
      return false;
    }
    delay(100);
  }
  FrameLog.println("Wi-Fi connected; synchronizing clock...");
  // Certificate verification needs a valid clock. RTC retains time in sleep.
  configTime(0, 0, "pool.ntp.org", "time.cloudflare.com");
  const uint32_t timeStarted = millis();
  while (time(nullptr) < 1704067200) {
    if (millis() - timeStarted >= 15000) {
      FrameLog.println("Clock sync failed; keeping existing picture.");
      return false;
    }
    delay(100);
  }
  return true;
}

static bool downloadFrame() {
  WiFiClientSecure tls;
  tls.setCACert(ROOT_CA);
  tls.setHandshakeTimeout(15);
  tls.setTimeout(10000);
  HTTPClient http;
  http.useHTTP10(true);  // Request a length-delimited response, not chunked.
  http.setConnectTimeout(10000);
  http.setTimeout(10000);
  // Use the direct R2 URL; do not follow redirects to another host.
  if (!http.begin(tls, IMAGE_URL)) return false;
  http.addHeader("Cache-Control", "no-cache");
  FrameLog.println("Downloading image over HTTPS...");
  const int status = http.GET();
  const int size = http.getSize();
  if (status != HTTP_CODE_OK || size != static_cast<int>(FRAME_BYTES)) {
    FrameLog.printf("Rejected response: HTTP %d, bytes %d (expected %u)\n",
                  status, size, static_cast<unsigned>(FRAME_BYTES));
    http.end();
    return false;
  }
  auto *stream = http.getStreamPtr();
  size_t received = 0;
  const uint32_t started = millis();
  uint32_t lastData = started;
  while (received < FRAME_BYTES) {
    const int available = stream->available();
    if (available > 0) {
      size_t count = static_cast<size_t>(available);
      if (count > FRAME_BYTES - received) count = FRAME_BYTES - received;
      if (count > 4096) count = 4096;
      const int got = stream->read(frame + received, count);
      if (got > 0) {
        received += static_cast<size_t>(got);
        lastData = millis();
      }
    } else if (!stream->connected()) {
      break;
    }
    if (millis() - started >= DOWNLOAD_TIMEOUT_MS ||
        millis() - lastData >= 10000) break;
    delay(1);
  }
  http.end();
  if (received != FRAME_BYTES) {
    FrameLog.printf("Incomplete download: %u bytes; keeping existing picture.\n",
                  static_cast<unsigned>(received));
    return false;
  }
  // Only the six panel color codes are valid. No refresh before validation.
  constexpr uint16_t allowed = (1 << 0) | (1 << 1) | (1 << 2) |
                               (1 << 3) | (1 << 5) | (1 << 6);
  for (size_t i = 0; i < FRAME_BYTES; ++i) {
    if (!(allowed & (1 << (frame[i] >> 4))) ||
        !(allowed & (1 << (frame[i] & 15)))) {
      FrameLog.printf("Invalid pixel data at byte %u\n", static_cast<unsigned>(i));
      return false;
    }
  }
  return true;
}

static uint8_t swapNibbles(uint8_t value) {
  return static_cast<uint8_t>((value << 4) | (value >> 4));
}

static void rotate180() {
  // Reverse pixel order: reverse bytes and swap the two pixels in each byte.
  for (size_t i = 0; i < FRAME_BYTES / 2; ++i) {
    const size_t j = FRAME_BYTES - 1 - i;
    const uint8_t a = frame[i];
    frame[i] = swapNibbles(frame[j]);
    frame[j] = swapNibbles(a);
  }
}

static void renderFrame() {
  if (ROTATE_180) rotate180();
  // Power the display only after a complete, validated download.
  DEV_Module_Init();
  delay(100);
  FrameLog.println("Initializing display...");
  EPD_13IN3E_Init();
  EPD_13IN3E_Display(frame);
  if (EPD_13IN3E_Ok()) {
    EPD_13IN3E_Sleep();
    FrameLog.println("Refresh complete.");
  } else {
    FrameLog.println("Panel BUSY timeout; check display connection and power.");
  }
  powerOffPanel();
}

void setup() {
  gpio_deep_sleep_hold_dis();
  gpio_hold_dis(static_cast<gpio_num_t>(EPD_PWR_PIN));
  gpio_hold_dis(static_cast<gpio_num_t>(EPD_RST_PIN));
  FrameLog.begin(115200);
  delay(200);
  pinMode(EPD_PWR_PIN, OUTPUT);
  pinMode(EPD_RST_PIN, OUTPUT);
  powerOffPanel();
  FrameLog.println("Hourly EINK frame starting");
  FrameLog.printf("PSRAM found: %s, size: %lu bytes\n",
                  psramFound() ? "yes" : "no", ESP.getPsramSize());
  if (!psramFound()) {
    FrameLog.println("PSRAM missing: select OPI PSRAM in Arduino Tools.");
    sleepUntilNextHour();
  }
  frame = static_cast<uint8_t *>(heap_caps_malloc(
      FRAME_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (!frame) {
    FrameLog.println("Frame allocation failed.");
    sleepUntilNextHour();
  }
  if (!connectWiFi() || !downloadFrame()) sleepUntilNextHour();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  renderFrame();
  sleepUntilNextHour();
}

void loop() {} // Timer wake starts setup() again.
