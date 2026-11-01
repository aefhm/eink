#pragma once

// Edit these locally before uploading. ESP32 Wi-Fi uses 2.4 GHz.
constexpr char WIFI_SSID[] = "YOUR_WIFI_NAME";
constexpr char WIFI_PASSWORD[] = "YOUR_WIFI_PASSWORD";
constexpr char IMAGE_URL[] = "https://example.org";

// The supplied file is already 1200 x 1600 portrait, in panel pixel order.
// Set true only if it appears upside down on your mounted frame.
constexpr bool ROTATE_180 = false;
constexpr uint64_t REFRESH_SECONDS = 3600;
