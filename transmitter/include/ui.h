#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Arduino.h>
#include <SPI.h>

#include "board_pins.h"

namespace pfx {

struct UiTelemetry {
  bool linkConnected = false;
  int16_t rssiDbm = -127;
  int16_t snrTenths = INT16_MIN;
  uint32_t linkAgeMs = UINT32_MAX;
  uint16_t latencyMs = UINT16_MAX;
  int batteryPercent = -1;
  bool radioReady = false;
  bool commandConfirmed = false;
  uint32_t packetCount = 0;
  uint32_t faultCount = 0;
  bool failsafeActive = false;
};

class Ui {
 public:
  Ui();

  // Call before Serial or any long initialization. This immediately drives
  // the backlight LOW so the TFT cannot show a white or partially drawn frame.
  void begin();
  void drawBoot();
  void setBootProgress(uint8_t percent, const __FlashStringHelper* message);
  void startMain(uint8_t relayMask, const UiTelemetry& telemetry);
  void setRelayMaskImmediate(uint8_t relayMask);
  void update(uint8_t relayMask, const UiTelemetry& telemetry);
  void showFatal(const __FlashStringHelper* title, int errorCode);

 private:
  SPIClass displaySpi_;
  Adafruit_ST7789 tft_;

  uint8_t previousMask_ = 0xFF;
  uint8_t previousStripMask_ = 0xFF;
  bool previousLink_ = false;
  bool previousRadioReady_ = false;
  bool previousConfirmed_ = false;
  bool previousFailsafe_ = false;
  int16_t previousRssi_ = 32767;
  int16_t previousSnr_ = 32767;
  uint16_t previousAgeBucket_ = 0xFFFF;
  uint16_t previousLatencyBucket_ = 0xFFFF;
  int previousBattery_ = -99;
  uint32_t previousPacketBucket_ = UINT32_MAX;
  uint32_t previousFaultCount_ = UINT32_MAX;
  uint32_t lastStatusDrawAt_ = 0;
  uint32_t lastTransportDrawAt_ = 0;
  uint32_t lastStripDrawAt_ = 0;

  static constexpr uint16_t kBlack = 0x0000;
  static constexpr uint16_t kPanel = 0x0000;
  static constexpr uint16_t kPanel2 = 0x1945;
  static constexpr uint16_t kWhite = 0xF7BF;
  static constexpr uint16_t kMuted = 0x9492;
  static constexpr uint16_t kOrange = 0xFB80;
  static constexpr uint16_t kCyan = 0x05FF;
  static constexpr uint16_t kGreen = 0x1F27;
  static constexpr uint16_t kRed = 0xF9E6;
  static constexpr uint16_t kAmber = 0xFCE2;

  void setBacklight(bool enabled);
  void pushBitmap(int16_t x, int16_t y, int16_t width, int16_t height,
                  const uint16_t* pixels);
  void resetCaches();
  void drawMainFrame();
  void drawStatus(const UiTelemetry& telemetry);
  void drawRelayCard(uint8_t index, bool active);
  void drawTransport(const UiTelemetry& telemetry);
  void drawActiveStrip(uint8_t relayMask, const UiTelemetry& telemetry);
  void drawBattery(int16_t x, int16_t y, int percent, uint16_t color);
  void drawGfxCentered(int16_t x, int16_t y, int16_t width,
                       const String& text, uint16_t color,
                       uint16_t background = kBlack);
  void drawTinyText(int16_t x, int16_t y, const String& text,
                    uint16_t color, uint8_t scale = 1);
  int16_t tinyTextWidth(const String& text, uint8_t scale) const;
};

}  // namespace pfx
