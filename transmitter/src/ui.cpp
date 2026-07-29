#include "ui.h"

#include "generated/boot_screen_rgb565.h"
#include "generated/main_chrome_rgb565.h"
#include "generated/relay_cards_rgb565.h"

namespace pfx {
namespace {

constexpr int16_t kScreenWidth = 170;
constexpr int16_t kScreenHeight = 320;
constexpr int16_t kCardWidth = 52;
constexpr int16_t kCardHeight = 64;
constexpr int16_t kCardX[3] = {5, 60, 115};
constexpr int16_t kCardY[2] = {165, 232};
constexpr uint32_t kDisplaySpiHz = 40000000UL;

static_assert(PFX_BOOT_SCREEN_WIDTH == kScreenWidth &&
              PFX_BOOT_SCREEN_HEIGHT == kScreenHeight,
              "Boot bitmap dimensions must match the T190 display");
static_assert(PFX_MAIN_CHROME_WIDTH == kScreenWidth &&
              PFX_MAIN_CHROME_HEIGHT == kScreenHeight,
              "Main bitmap dimensions must match the T190 display");
static_assert(PFX_RELAY_1_OFF_WIDTH == kCardWidth &&
              PFX_RELAY_1_OFF_HEIGHT == kCardHeight,
              "Relay card dimensions do not match UI coordinates");
static_assert(PFX_RELAY_6_ON_WIDTH == kCardWidth &&
              PFX_RELAY_6_ON_HEIGHT == kCardHeight,
              "Relay card dimensions do not match UI coordinates");

const uint16_t* offCardFor(uint8_t index) {
  switch (index) {
    case 0: return PFX_RELAY_1_OFF;
    case 1: return PFX_RELAY_2_OFF;
    case 2: return PFX_RELAY_3_OFF;
    case 3: return PFX_RELAY_4_OFF;
    case 4: return PFX_RELAY_5_OFF;
    default: return PFX_RELAY_6_OFF;
  }
}

const uint16_t* onCardFor(uint8_t index) {
  switch (index) {
    case 0: return PFX_RELAY_1_ON;
    case 1: return PFX_RELAY_2_ON;
    case 2: return PFX_RELAY_3_ON;
    case 3: return PFX_RELAY_4_ON;
    case 4: return PFX_RELAY_5_ON;
    default: return PFX_RELAY_6_ON;
  }
}

uint16_t ageBucket(uint32_t ageMs) {
  if (ageMs == UINT32_MAX) return 0xFFFF;
  if (ageMs < 10000U) return static_cast<uint16_t>(ageMs / 100U);
  const uint32_t seconds = ageMs / 1000U;
  return static_cast<uint16_t>(100U + (seconds > 999U ? 999U : seconds));
}

uint16_t latencyBucket(uint16_t latencyMs) {
  if (latencyMs == UINT16_MAX) return 0xFFFF;
  return latencyMs > 999U ? 999U : latencyMs;
}

uint8_t activeCount(uint8_t mask) {
  mask &= 0x3F;
  uint8_t count = 0;
  while (mask) {
    count += mask & 1U;
    mask >>= 1U;
  }
  return count;
}

uint8_t firstActiveChannel(uint8_t mask) {
  for (uint8_t i = 0; i < 6; ++i) {
    if (mask & static_cast<uint8_t>(1U << i)) return i + 1U;
  }
  return 0;
}

String activeValue(uint8_t relayMask) {
  const uint8_t count = activeCount(relayMask);
  if (count == 0) return "NONE";
  if (count == 1) return String("CH") + String(firstActiveChannel(relayMask));
  return String(count) + "CH";
}

String compactAge(uint32_t ageMs) {
  if (ageMs == UINT32_MAX) return "---";
  if (ageMs < 1000U) return String(".") + String((ageMs / 100U) % 10U) + "S";
  if (ageMs < 10000U) {
    return String(ageMs / 1000U) + "." + String((ageMs / 100U) % 10U) + "S";
  }
  const uint32_t seconds = ageMs / 1000U;
  return String(seconds > 99U ? 99U : seconds) + "S";
}

void glyphRows(char c, uint8_t rows[5]) {
  for (uint8_t i = 0; i < 5; ++i) rows[i] = 0;
  switch (c) {
    case 'A': { const uint8_t r[5]={2,5,7,5,5}; memcpy(rows,r,5); break; }
    case 'B': { const uint8_t r[5]={6,5,6,5,6}; memcpy(rows,r,5); break; }
    case 'C': { const uint8_t r[5]={3,4,4,4,3}; memcpy(rows,r,5); break; }
    case 'D': { const uint8_t r[5]={6,5,5,5,6}; memcpy(rows,r,5); break; }
    case 'E': { const uint8_t r[5]={7,4,6,4,7}; memcpy(rows,r,5); break; }
    case 'F': { const uint8_t r[5]={7,4,6,4,4}; memcpy(rows,r,5); break; }
    case 'G': { const uint8_t r[5]={3,4,5,5,3}; memcpy(rows,r,5); break; }
    case 'H': { const uint8_t r[5]={5,5,7,5,5}; memcpy(rows,r,5); break; }
    case 'I': { const uint8_t r[5]={7,2,2,2,7}; memcpy(rows,r,5); break; }
    case 'J': { const uint8_t r[5]={1,1,1,5,2}; memcpy(rows,r,5); break; }
    case 'K': { const uint8_t r[5]={5,5,6,5,5}; memcpy(rows,r,5); break; }
    case 'L': { const uint8_t r[5]={4,4,4,4,7}; memcpy(rows,r,5); break; }
    case 'M': { const uint8_t r[5]={5,7,7,5,5}; memcpy(rows,r,5); break; }
    case 'N': { const uint8_t r[5]={5,7,7,7,5}; memcpy(rows,r,5); break; }
    case 'O': { const uint8_t r[5]={2,5,5,5,2}; memcpy(rows,r,5); break; }
    case 'P': { const uint8_t r[5]={6,5,6,4,4}; memcpy(rows,r,5); break; }
    case 'Q': { const uint8_t r[5]={2,5,5,3,1}; memcpy(rows,r,5); break; }
    case 'R': { const uint8_t r[5]={6,5,6,5,5}; memcpy(rows,r,5); break; }
    case 'S': { const uint8_t r[5]={3,4,2,1,6}; memcpy(rows,r,5); break; }
    case 'T': { const uint8_t r[5]={7,2,2,2,2}; memcpy(rows,r,5); break; }
    case 'U': { const uint8_t r[5]={5,5,5,5,7}; memcpy(rows,r,5); break; }
    case 'V': { const uint8_t r[5]={5,5,5,5,2}; memcpy(rows,r,5); break; }
    case 'W': { const uint8_t r[5]={5,5,7,7,5}; memcpy(rows,r,5); break; }
    case 'X': { const uint8_t r[5]={5,5,2,5,5}; memcpy(rows,r,5); break; }
    case 'Y': { const uint8_t r[5]={5,5,2,2,2}; memcpy(rows,r,5); break; }
    case 'Z': { const uint8_t r[5]={7,1,2,4,7}; memcpy(rows,r,5); break; }
    case '0': { const uint8_t r[5]={7,5,5,5,7}; memcpy(rows,r,5); break; }
    case '1': { const uint8_t r[5]={2,6,2,2,7}; memcpy(rows,r,5); break; }
    case '2': { const uint8_t r[5]={6,1,7,4,7}; memcpy(rows,r,5); break; }
    case '3': { const uint8_t r[5]={6,1,3,1,6}; memcpy(rows,r,5); break; }
    case '4': { const uint8_t r[5]={5,5,7,1,1}; memcpy(rows,r,5); break; }
    case '5': { const uint8_t r[5]={7,4,6,1,6}; memcpy(rows,r,5); break; }
    case '6': { const uint8_t r[5]={3,4,7,5,7}; memcpy(rows,r,5); break; }
    case '7': { const uint8_t r[5]={7,1,2,2,2}; memcpy(rows,r,5); break; }
    case '8': { const uint8_t r[5]={7,5,7,5,7}; memcpy(rows,r,5); break; }
    case '9': { const uint8_t r[5]={7,5,7,1,6}; memcpy(rows,r,5); break; }
    case ':': { const uint8_t r[5]={0,2,0,2,0}; memcpy(rows,r,5); break; }
    case '.': { const uint8_t r[5]={0,0,0,0,2}; memcpy(rows,r,5); break; }
    case '-': { const uint8_t r[5]={0,0,7,0,0}; memcpy(rows,r,5); break; }
    case '+': { const uint8_t r[5]={0,2,7,2,0}; memcpy(rows,r,5); break; }
    case '%': { const uint8_t r[5]={5,1,2,4,5}; memcpy(rows,r,5); break; }
    default: break;
  }
}

}  // namespace

Ui::Ui()
    : displaySpi_(HSPI),
      tft_(&displaySpi_, pins::TFT_CS, pins::TFT_DC, pins::TFT_RST) {}

void Ui::setBacklight(bool enabled) {
  digitalWrite(pins::TFT_BACKLIGHT, enabled ? HIGH : LOW);
}

void Ui::begin() {
  // Drive the backlight off before enabling the TFT power rail. This is the
  // first operation in setup() and removes the multi-second white flash.
  pinMode(pins::TFT_BACKLIGHT, OUTPUT);
  digitalWrite(pins::TFT_BACKLIGHT, LOW);
  pinMode(pins::TFT_POWER, OUTPUT);
  digitalWrite(pins::TFT_POWER, LOW);
  delay(5);

  // The previous builds used the software-SPI constructor, which visibly
  // painted the screen line by line. The TFT now uses the ESP32-S3 HSPI host;
  // the SX1262 remains on the default FSPI host.
  displaySpi_.begin(pins::TFT_SCLK, -1, pins::TFT_MOSI, pins::TFT_CS);
  tft_.init(kScreenWidth, kScreenHeight, SPI_MODE0);
  tft_.setSPISpeed(kDisplaySpiHz);
  tft_.setRotation(0);
  tft_.setTextWrap(false);
  tft_.fillScreen(kBlack);
}

void Ui::pushBitmap(int16_t x, int16_t y, int16_t width, int16_t height,
                    const uint16_t* pixels) {
  tft_.startWrite();
  tft_.setAddrWindow(x, y, width, height);
  tft_.writePixels(const_cast<uint16_t*>(pixels),
                   static_cast<uint32_t>(width) * static_cast<uint32_t>(height),
                   true, false);
  tft_.endWrite();
}

void Ui::drawBoot() {
  setBacklight(false);
  pushBitmap(0, 0, kScreenWidth, kScreenHeight, PFX_BOOT_SCREEN);
  delay(2);
  setBacklight(true);
}

void Ui::setBootProgress(uint8_t percent, const __FlashStringHelper* message) {
  percent = constrain(percent, static_cast<uint8_t>(0), static_cast<uint8_t>(100));

  // Dedicated progress panel at the very bottom of the molten background.
  // It never crosses the icon, wordmark, or role label.
  constexpr int16_t panelX = 9;
  constexpr int16_t panelY = 282;
  constexpr int16_t panelW = 152;
  constexpr int16_t panelH = 29;
  constexpr int16_t barX = 15;
  constexpr int16_t barY = 302;
  constexpr int16_t barW = 140;
  constexpr int16_t barH = 4;

  tft_.fillRoundRect(panelX, panelY, panelW, panelH, 5, kBlack);
  tft_.drawRoundRect(panelX, panelY, panelW, panelH, 5, kOrange);
  tft_.fillRect(barX, barY, barW, barH, kPanel2);
  const int16_t filled = static_cast<int16_t>((static_cast<uint32_t>(barW) * percent) / 100U);
  if (filled > 0) {
    tft_.fillRect(barX, barY, filled, barH, percent < 100 ? kOrange : kGreen);
  }

  tft_.fillRect(14, 287, 142, 11, kBlack);
  tft_.setTextSize(1);
  tft_.setTextColor(kWhite, kBlack);
  tft_.setCursor(15, 288);
  tft_.print(message);

  char progress[6];
  snprintf(progress, sizeof(progress), "%u%%", static_cast<unsigned>(percent));
  int16_t bx, by;
  uint16_t bw, bh;
  tft_.getTextBounds(progress, 0, 0, &bx, &by, &bw, &bh);
  tft_.setTextColor(percent < 100 ? kOrange : kGreen, kBlack);
  tft_.setCursor(154 - static_cast<int16_t>(bw), 288);
  tft_.print(progress);
}

void Ui::resetCaches() {
  previousMask_ = 0xFF;
  previousStripMask_ = 0xFF;
  previousLink_ = false;
  previousRadioReady_ = false;
  previousConfirmed_ = false;
  previousFailsafe_ = false;
  previousRssi_ = 32767;
  previousSnr_ = 32767;
  previousAgeBucket_ = 0xFFFF;
  previousLatencyBucket_ = 0xFFFF;
  previousBattery_ = -99;
  previousPacketBucket_ = UINT32_MAX;
  previousFaultCount_ = UINT32_MAX;
  lastStatusDrawAt_ = 0;
  lastTransportDrawAt_ = 0;
  lastStripDrawAt_ = 0;
}

void Ui::drawMainFrame() {
  pushBitmap(0, 0, kScreenWidth, kScreenHeight, PFX_MAIN_CHROME);
  resetCaches();
}

void Ui::startMain(uint8_t relayMask, const UiTelemetry& telemetry) {
  // Compose the complete initial frame while the LEDs are disabled. With the
  // hardware-SPI bulk transfers below, the screen is ready before it is shown.
  setBacklight(false);
  drawMainFrame();
  setRelayMaskImmediate(relayMask);
  drawStatus(telemetry);
  drawTransport(telemetry);
  drawActiveStrip(relayMask, telemetry);

  previousLink_ = telemetry.linkConnected;
  previousRadioReady_ = telemetry.radioReady;
  previousRssi_ = telemetry.rssiDbm;
  previousSnr_ = telemetry.snrTenths;
  previousAgeBucket_ = ageBucket(telemetry.linkAgeMs);
  previousLatencyBucket_ = latencyBucket(telemetry.latencyMs);
  previousBattery_ = telemetry.batteryPercent;
  previousConfirmed_ = telemetry.commandConfirmed;
  previousFailsafe_ = telemetry.failsafeActive;
  previousPacketBucket_ = telemetry.packetCount / 10UL;
  previousFaultCount_ = telemetry.faultCount;
  previousStripMask_ = relayMask & 0x3F;
  const uint32_t now = millis();
  lastStatusDrawAt_ = now;
  lastTransportDrawAt_ = now;
  lastStripDrawAt_ = now;

  delay(2);
  setBacklight(true);
}

int16_t Ui::tinyTextWidth(const String& text, uint8_t scale) const {
  if (text.length() == 0) return 0;
  return static_cast<int16_t>(text.length() * 4U * scale - scale);
}

void Ui::drawTinyText(int16_t x, int16_t y, const String& text,
                      uint16_t color, uint8_t scale) {
  int16_t cursor = x;
  for (uint16_t i = 0; i < text.length(); ++i) {
    char c = text[i];
    if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
    uint8_t rows[5];
    glyphRows(c, rows);
    for (uint8_t row = 0; row < 5; ++row) {
      for (uint8_t col = 0; col < 3; ++col) {
        if (rows[row] & static_cast<uint8_t>(1U << (2U - col))) {
          tft_.fillRect(cursor + col * scale, y + row * scale,
                        scale, scale, color);
        }
      }
    }
    cursor += 4 * scale;
  }
}

void Ui::drawGfxCentered(int16_t x, int16_t y, int16_t width,
                         const String& text, uint16_t color,
                         uint16_t background) {
  int16_t bx, by;
  uint16_t bw, bh;
  tft_.setTextSize(1);
  tft_.getTextBounds(text, 0, 0, &bx, &by, &bw, &bh);
  tft_.setTextColor(color, background);
  const int16_t offset = width > static_cast<int16_t>(bw)
      ? (width - static_cast<int16_t>(bw)) / 2 : 0;
  tft_.setCursor(x + offset, y);
  tft_.print(text);
}

void Ui::drawBattery(int16_t x, int16_t y, int percent, uint16_t color) {
  // Keep the power indicator visibly green when running from USB with no LiPo
  // attached. A valid low battery still uses red.
  tft_.drawRoundRect(x, y, 13, 8, 1, color);
  tft_.fillRect(x + 13, y + 2, 2, 4, color);
  tft_.fillRect(x + 2, y + 2, 9, 4, kBlack);
  if (percent >= 0) {
    const int16_t width = constrain((9 * percent + 50) / 100, 0, 9);
    if (width > 0) tft_.fillRect(x + 2, y + 2, width, 4, color);
  } else {
    // Small lightning mark denotes external/USB power rather than inventing a
    // battery percentage when the ADC correctly reports no battery.
    tft_.drawFastVLine(x + 7, y + 2, 2, color);
    tft_.drawPixel(x + 6, y + 4, color);
    tft_.drawFastVLine(x + 5, y + 4, 2, color);
  }
}

void Ui::drawStatus(const UiTelemetry& t) {
  const bool connected = t.radioReady && t.linkConnected;
  const uint16_t liveColor = !t.radioReady ? kRed : (connected ? kGreen : kAmber);
  tft_.fillRect(0, 56, 170, 14, kBlack);
  tft_.drawFastVLine(34, 56, 14, kMuted);
  tft_.drawFastVLine(68, 56, 14, kMuted);
  tft_.drawFastVLine(102, 56, 14, kMuted);
  tft_.drawFastVLine(136, 56, 14, kMuted);

  drawGfxCentered(0, 61, 34,
      !t.radioReady ? String("ERR") : (connected ? String("LIVE") : String("WAIT")),
      liveColor);

  String rssi = "---";
  if (connected && t.rssiDbm > -140) {
    rssi = String(constrain(static_cast<int>(t.rssiDbm), -139, 0));
  }
  drawGfxCentered(34, 61, 34, rssi, connected ? kGreen : kAmber);

  String snr = "---";
  if (connected && t.snrTenths != INT16_MIN) {
    const int16_t rounded = static_cast<int16_t>(
        (t.snrTenths + (t.snrTenths >= 0 ? 5 : -5)) / 10);
    snr = String(rounded);
  }
  drawGfxCentered(68, 61, 34, snr, connected ? kGreen : kAmber);
  drawGfxCentered(102, 61, 34,
                  connected ? compactAge(t.linkAgeMs) : String("---"),
                  connected ? kGreen : kAmber);

  const uint16_t batteryColor =
      t.batteryPercent >= 0 && t.batteryPercent < 20 ? kRed : kGreen;
  drawBattery(138, 61, t.batteryPercent, batteryColor);
  tft_.fillRect(153, 56, 17, 14, kBlack);
  if (t.batteryPercent >= 0) {
    tft_.setTextSize(1);
    tft_.setTextColor(batteryColor, kBlack);
    tft_.setCursor(153, 61);
    tft_.print(constrain(t.batteryPercent, 0, 99));
    tft_.print('%');
  } else {
    drawTinyText(154, 62, "USB", kGreen, 1);
  }
}

void Ui::drawRelayCard(uint8_t index, bool active) {
  if (index >= 6) return;
  const int16_t x = kCardX[index % 3U];
  const int16_t y = kCardY[index / 3U];
  pushBitmap(x, y, kCardWidth, kCardHeight,
             active ? onCardFor(index) : offCardFor(index));
}

void Ui::drawTransport(const UiTelemetry& t) {
  tft_.fillRect(8, 113, 154, 14, kBlack);
  tft_.drawFastVLine(46, 113, 14, kMuted);
  tft_.drawFastVLine(85, 113, 14, kMuted);
  tft_.drawFastVLine(124, 113, 14, kMuted);
  drawGfxCentered(7, 116, 39, String(t.packetCount % 10000UL), kCyan, kBlack);
#if defined(PFX_ROLE_SENDER)
  const String state = !t.radioReady ? "ERR" : (!t.linkConnected ? "WAIT"
      : (t.commandConfirmed ? "OK" : "PEND"));
  const uint16_t stateColor = !t.radioReady ? kRed
      : (!t.linkConnected ? kAmber : (t.commandConfirmed ? kGreen : kAmber));
#else
  const String state = t.failsafeActive ? "SAFE" : (!t.radioReady ? "ERR"
      : (t.linkConnected ? "LIVE" : "WAIT"));
  const uint16_t stateColor = t.failsafeActive || !t.radioReady ? kRed
      : (t.linkConnected ? kGreen : kAmber);
#endif
  drawGfxCentered(46, 116, 39, state, stateColor, kBlack);
  drawGfxCentered(85, 116, 39, String(t.faultCount % 100UL),
                  t.faultCount ? kAmber : kCyan, kBlack);

  const uint32_t totalSeconds = millis() / 1000UL;
  const uint32_t minutes = (totalSeconds / 60UL) % 100UL;
  const uint32_t seconds = totalSeconds % 60UL;
  char uptime[6];
  snprintf(uptime, sizeof(uptime), "%02lu:%02lu",
           static_cast<unsigned long>(minutes),
           static_cast<unsigned long>(seconds));
  drawGfxCentered(124, 116, 39, String(uptime), kCyan, kBlack);
}

void Ui::drawActiveStrip(uint8_t relayMask, const UiTelemetry& t) {
  relayMask &= 0x3F;
  tft_.fillRect(7, 136, 156, 12, kBlack);
#if defined(PFX_ROLE_SENDER)
  tft_.drawFastVLine(48, 136, 12, kMuted);
  tft_.drawFastVLine(84, 136, 12, kMuted);
  tft_.drawFastVLine(122, 136, 12, kMuted);
#else
  tft_.drawFastVLine(52, 136, 12, kMuted);
  tft_.drawFastVLine(90, 136, 12, kMuted);
  tft_.drawFastVLine(128, 136, 12, kMuted);
#endif
  drawTinyText(8, 139, "ACT:", kWhite, 1);
  drawTinyText(24, 139, activeValue(relayMask), relayMask ? kOrange : kMuted, 1);

#if defined(PFX_ROLE_SENDER)
  drawTinyText(51, 139, "ACK:", kWhite, 1);
  const String ack = !t.radioReady ? "ERR" : (!t.linkConnected ? "WAIT"
      : (t.commandConfirmed ? "OK" : "PEND"));
  drawTinyText(67, 139, ack,
               t.commandConfirmed ? kGreen : (t.linkConnected ? kAmber : kMuted), 1);
  drawTinyText(87, 139, "LAT:", kWhite, 1);
  const String latency = t.latencyMs == UINT16_MAX ? "---" :
      String(t.latencyMs > 999U ? 999U : t.latencyMs) + "MS";
  drawTinyText(103, 139, latency, t.linkConnected ? kOrange : kMuted, 1);
  drawTinyText(125, 139, "RX:", kWhite, 1);
  drawTinyText(137, 139,
               t.linkConnected ? compactAge(t.linkAgeMs) : String("---"),
               t.linkConnected ? kGreen : kMuted, 1);
#else
  drawTinyText(55, 139, "RX:", kWhite, 1);
  const String rxState = t.failsafeActive ? "SAFE" : (!t.radioReady ? "ERR"
      : (t.linkConnected ? "LIVE" : "WAIT"));
  drawTinyText(67, 139, rxState,
               t.failsafeActive ? kRed : (t.linkConnected ? kGreen : kMuted), 1);
  drawTinyText(92, 139, "RSSI:", kWhite, 1);
  drawTinyText(112, 139,
               t.linkConnected ? String(t.rssiDbm) : String("---"),
               t.linkConnected ? kGreen : kMuted, 1);
  drawTinyText(131, 139, "AGE:", kWhite, 1);
  drawTinyText(147, 139,
               t.linkConnected ? compactAge(t.linkAgeMs) : String("---"),
               t.linkConnected ? kGreen : kMuted, 1);
#endif
}

void Ui::setRelayMaskImmediate(uint8_t relayMask) {
  relayMask &= 0x3F;
  if (previousMask_ == relayMask) return;
  for (uint8_t i = 0; i < 6; ++i) {
    const bool oldState = previousMask_ != 0xFF && ((previousMask_ >> i) & 1U);
    const bool newState = ((relayMask >> i) & 1U) != 0;
    if (previousMask_ == 0xFF || oldState != newState) {
      drawRelayCard(i, newState);
    }
  }
  previousMask_ = relayMask;
}

void Ui::update(uint8_t relayMask, const UiTelemetry& telemetry) {
  relayMask &= 0x3F;
  setRelayMaskImmediate(relayMask);

  const uint32_t now = millis();
  const uint16_t age = ageBucket(telemetry.linkAgeMs);
  const uint16_t latency = latencyBucket(telemetry.latencyMs);
  const bool linkChanged = previousLink_ != telemetry.linkConnected ||
                           previousRadioReady_ != telemetry.radioReady;
  const bool ageChanged = previousAgeBucket_ != age;
  const bool statusChanged = linkChanged ||
      previousRssi_ != telemetry.rssiDbm ||
      previousSnr_ != telemetry.snrTenths || ageChanged ||
      previousBattery_ != telemetry.batteryPercent;

  if (statusChanged && (linkChanged || now - lastStatusDrawAt_ >= 100U)) {
    drawStatus(telemetry);
    previousLink_ = telemetry.linkConnected;
    previousRadioReady_ = telemetry.radioReady;
    previousRssi_ = telemetry.rssiDbm;
    previousSnr_ = telemetry.snrTenths;
    previousAgeBucket_ = age;
    previousBattery_ = telemetry.batteryPercent;
    lastStatusDrawAt_ = now;
  }

  const bool maskChanged = previousStripMask_ != relayMask;
  if (maskChanged) previousStripMask_ = relayMask;

  const uint32_t packetBucket = telemetry.packetCount / 10UL;
  const bool transportStateChanged =
      previousConfirmed_ != telemetry.commandConfirmed ||
      previousFailsafe_ != telemetry.failsafeActive ||
      previousFaultCount_ != telemetry.faultCount || linkChanged;
  const bool transportCountChanged = previousPacketBucket_ != packetBucket;
  if (transportStateChanged ||
      (transportCountChanged && now - lastTransportDrawAt_ >= 200U)) {
    drawTransport(telemetry);
    previousConfirmed_ = telemetry.commandConfirmed;
    previousFailsafe_ = telemetry.failsafeActive;
    previousFaultCount_ = telemetry.faultCount;
    previousPacketBucket_ = packetBucket;
    lastTransportDrawAt_ = now;
  }

  const bool stripChanged = maskChanged || transportStateChanged ||
      previousLatencyBucket_ != latency || ageChanged;
  if (stripChanged &&
      (maskChanged || transportStateChanged || now - lastStripDrawAt_ >= 100U)) {
    drawActiveStrip(relayMask, telemetry);
    previousLatencyBucket_ = latency;
    lastStripDrawAt_ = now;
  }
}

void Ui::showFatal(const __FlashStringHelper* title, int errorCode) {
  setBacklight(false);
  tft_.fillScreen(kBlack);
  tft_.setTextSize(2);
  tft_.setTextColor(kWhite, kBlack);
  tft_.setCursor(14, 35);
  tft_.print(F("PYROTECHNIC"));
  tft_.setTextColor(kOrange, kBlack);
  tft_.print(F("FX"));
  tft_.drawRoundRect(10, 92, 150, 128, 10, kRed);
  tft_.setCursor(32, 110);
  tft_.print(F("RF ERROR"));
  tft_.setTextSize(1);
  tft_.setCursor(19, 151);
  tft_.print(title);
  tft_.setCursor(46, 174);
  tft_.print(F("CODE: "));
  tft_.print(errorCode);
  tft_.setTextColor(kMuted, kBlack);
  tft_.setCursor(21, 198);
  tft_.print(F("OUTPUTS REMAIN SAFE OFF"));
  delay(2);
  setBacklight(true);
}

}  // namespace pfx
