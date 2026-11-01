#pragma once
#include <Arduino.h>

// Mirror diagnostics to UART0 (USB bridge) and native USB when connected.
class FrameConsole : public Print {
public:
  using Print::write;

  void begin(unsigned long baud) {
    Serial0.begin(baud);
#if ARDUINO_USB_CDC_ON_BOOT
    Serial.begin(baud);
#endif
  }

  size_t write(uint8_t c) override { return write(&c, 1); }

  size_t write(const uint8_t *buffer, size_t size) override {
    Serial0.write(buffer, size);
#if ARDUINO_USB_CDC_ON_BOOT
    if (Serial) Serial.write(buffer, size);
#endif
    return size;
  }

  void flush() {
    Serial0.flush();
#if ARDUINO_USB_CDC_ON_BOOT
    if (Serial) Serial.flush();
#endif
  }
};

inline FrameConsole FrameLog;
