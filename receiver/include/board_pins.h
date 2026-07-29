#pragma once
#include <Arduino.h>

namespace pfx::pins {

// Heltec Vision Master T190 1.9-inch ST7789 TFT.
constexpr int TFT_SCLK = 38;
constexpr int TFT_MOSI = 48;
constexpr int TFT_CS = 39;
constexpr int TFT_DC = 47;
constexpr int TFT_RST = 40;
constexpr int TFT_BACKLIGHT = 17;
constexpr int TFT_POWER = 7;  // LOW enables the TFT power rail.

// Heltec Vision Master T190 SX1262 / HT-RA62.
constexpr int LORA_SCK = 9;
constexpr int LORA_MISO = 11;
constexpr int LORA_MOSI = 10;
constexpr int LORA_NSS = 8;
constexpr int LORA_RST = 12;
constexpr int LORA_BUSY = 13;
constexpr int LORA_DIO1 = 14;

// Battery measurement circuit.
constexpr int BATTERY_ADC_ENABLE = 5;
constexpr int BATTERY_ADC = 6;

// Six receiver relay control outputs. GPIO15 and GPIO16 are exposed on J2
// and do not conflict with the TFT, battery monitor, or SX1262 connections.
constexpr uint8_t RECEIVER_RELAYS[6] = {1, 2, 3, 4, 15, 16};

}  // namespace pfx::pins
