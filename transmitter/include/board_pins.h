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

// Relay 1 uses the onboard USER button on GPIO21.
// Relays 2-6 use external normally-open momentary buttons to GND.
// GPIO15 is exposed on the T190 J2 header and does not conflict with the TFT,
// battery monitor, or SX1262 connections used by this project.
constexpr uint8_t SENDER_BUTTONS[6] = {21, 1, 2, 3, 4, 15};

}  // namespace pfx::pins
