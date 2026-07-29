#pragma once
#include <Arduino.h>

namespace pfx::config {

// Both T190 units must use identical radio settings.
constexpr float RF_FREQUENCY_MHZ = 915.0F;
constexpr float RF_BANDWIDTH_KHZ = 250.0F;
constexpr uint8_t RF_SPREADING_FACTOR = 7;
constexpr uint8_t RF_CODING_RATE = 5;
constexpr int8_t RF_OUTPUT_POWER_DBM = 14;
constexpr uint16_t RF_PREAMBLE_SYMBOLS = 8;
constexpr float RF_TCXO_VOLTAGE = 1.8F;

constexpr uint32_t SYSTEM_ID = 0x50465831UL;
constexpr uint32_t SHARED_KEY = 0x8A63D14BUL;

// Latest-state-wins radio scheduling. There is no blocking ACK wait and no
// stale burst queue. An edge transmits immediately, then one extra copy is
// sent only if the command generation has not changed.
constexpr uint32_t ACTIVE_REPEAT_MS = 55;
constexpr uint32_t IDLE_REPEAT_MS = 250;
constexpr uint32_t EDGE_REPEAT_GAP_MS = 12;
constexpr uint8_t EDGE_EXTRA_COPIES = 1;
constexpr uint32_t RECEIVER_FAILSAFE_MS = 500;
constexpr uint32_t LINK_LOST_MS = 900;

// Momentary/dead-man input timing. PRESS needs one stable millisecond to avoid
// electrical chatter. RELEASE is accepted on the first HIGH sample.
constexpr uint32_t BUTTON_PRESS_DEBOUNCE_MS = 1;
constexpr uint32_t BUTTON_REPRESS_GUARD_MS = 2;

constexpr uint32_t BOOT_SPLASH_MS = 2000;
constexpr bool RELAY_ACTIVE_LOW = true;
constexpr float BATTERY_EMPTY_V = 3.35F;
constexpr float BATTERY_FULL_V = 4.18F;

}  // namespace pfx::config
