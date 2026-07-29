#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include "board_pins.h"
#include "config.h"
#include "protocol.h"
#include "radio_support.h"
#include "ui.h"

namespace {

using pfx::protocol::Packet;
using pfx::protocol::PacketType;
constexpr uint8_t kRelayCount = 6;
constexpr uint8_t kRelayMask = 0x3F;

pfx::Ui ui;

void showBootStage(uint8_t percent, const __FlashStringHelper* message,
                   uint32_t minimumVisibleMs) {
  ui.setBootProgress(percent, message);
  const uint32_t shownAt = millis();
  while (millis() - shownAt < minimumVisibleMs) {
    delay(10);
  }
}
bool radioReady = false;
TaskHandle_t radioTaskHandle = nullptr;
TaskHandle_t inputTaskHandle = nullptr;
QueueHandle_t logQueue = nullptr;
TaskHandle_t loggerTaskHandle = nullptr;
portMUX_TYPE sharedMux = portMUX_INITIALIZER_UNLOCKED;

volatile bool radioPacketFlag = false;
volatile bool receiveArmed = false;

struct DesiredSnapshot {
  uint8_t mask;
  uint32_t generation;
  uint32_t changedAt;
};

class ResponsiveButtons {
 public:
  void begin() {
    const uint32_t now = millis();
    for (uint8_t i = 0; i < kRelayCount; ++i) {
      pinMode(pfx::pins::SENDER_BUTTONS[i], INPUT_PULLUP);
      const bool pressed = digitalRead(pfx::pins::SENDER_BUTTONS[i]) == LOW;
      raw_[i] = pressed;
      stable_[i] = pressed;
      changedAt_[i] = now;
      releasedAt_[i] = pressed ? 0 : now;
    }
  }

  uint8_t update() {
    const uint32_t now = millis();
    uint8_t mask = 0;

    for (uint8_t i = 0; i < kRelayCount; ++i) {
      const bool pressed = digitalRead(pfx::pins::SENDER_BUTTONS[i]) == LOW;

      if (stable_[i]) {
        // Dead-man behavior: RELEASE is accepted on the first HIGH sample.
        if (!pressed) {
          stable_[i] = false;
          raw_[i] = false;
          changedAt_[i] = now;
          releasedAt_[i] = now;
        }
      } else {
        if (pressed != raw_[i]) {
          raw_[i] = pressed;
          changedAt_[i] = now;
        }

        if (raw_[i]) {
          const bool guardElapsed =
              now - releasedAt_[i] >= pfx::config::BUTTON_REPRESS_GUARD_MS;
          const bool stableLongEnough =
              now - changedAt_[i] >= pfx::config::BUTTON_PRESS_DEBOUNCE_MS;
          if (guardElapsed && stableLongEnough) stable_[i] = true;
        }
      }

      if (stable_[i]) mask |= static_cast<uint8_t>(1U << i);
    }

    return mask & kRelayMask;
  }

 private:
  bool raw_[kRelayCount]{};
  bool stable_[kRelayCount]{};
  uint32_t changedAt_[kRelayCount]{};
  uint32_t releasedAt_[kRelayCount]{};
};

ResponsiveButtons buttons;

uint8_t desiredMask = 0;
uint32_t desiredGeneration = 0;
uint32_t desiredChangedAt = 0;
uint8_t confirmedMask = 0;
uint32_t lastAcknowledgementAt = 0;
uint32_t txPacketCount = 0;
uint32_t radioFaultCount = 0;
int currentBatteryPercent = -1;
int16_t lastRssi = -127;
int16_t lastSnrTenths = INT16_MIN;
uint16_t lastLatencyMs = UINT16_MAX;
bool commandPending = true;

struct LogMessage {
  char text[196];
};

void logLine(const char* line) {
  if (logQueue == nullptr) {
    Serial.println(line);
    return;
  }

  // Never perform USB/Serial writes in the input or RF timing paths. Queue a
  // complete line without waiting; the low-priority logger task prints it.
  LogMessage message{};
  strlcpy(message.text, line, sizeof(message.text));
  xQueueSend(logQueue, &message, 0);
}

void loggerWorker(void*) {
  LogMessage message{};
  for (;;) {
    if (xQueueReceive(logQueue, &message, portMAX_DELAY) == pdTRUE) {
      Serial.println(message.text);
    }
  }
}

DesiredSnapshot readDesiredSnapshot() {
  DesiredSnapshot snapshot{};
  portENTER_CRITICAL(&sharedMux);
  snapshot.mask = desiredMask;
  snapshot.generation = desiredGeneration;
  snapshot.changedAt = desiredChangedAt;
  portEXIT_CRITICAL(&sharedMux);
  return snapshot;
}

void publishDesiredMask(uint8_t mask, uint32_t detectedAt) {
  portENTER_CRITICAL(&sharedMux);
  desiredMask = mask & kRelayMask;
  desiredChangedAt = detectedAt;
  ++desiredGeneration;
  commandPending = true;
  portEXIT_CRITICAL(&sharedMux);

  if (radioTaskHandle != nullptr) xTaskNotifyGive(radioTaskHandle);
}

void logButtonTransitions(uint8_t oldMask, uint8_t newMask,
                          uint32_t detectedAt) {
  const uint8_t changed = oldMask ^ newMask;
  for (uint8_t i = 0; i < kRelayCount; ++i) {
    const uint8_t bit = static_cast<uint8_t>(1U << i);
    if ((changed & bit) == 0) continue;
    char line[128];
    snprintf(line, sizeof(line),
             "[BUTTON] detected=%lums relay=%u gpio=%u state=%s mask=0x%02X",
             static_cast<unsigned long>(detectedAt),
             static_cast<unsigned>(i + 1U),
             static_cast<unsigned>(pfx::pins::SENDER_BUTTONS[i]),
             (newMask & bit) ? "PRESSED" : "RELEASED",
             static_cast<unsigned>(newMask));
    logLine(line);
  }
}

pfx::UiTelemetry makeTelemetry(uint32_t now) {
  uint8_t desired;
  uint8_t confirmed;
  uint32_t ackAt;
  uint32_t packets;
  uint32_t faults;
  int battery;
  int16_t rssi;
  int16_t snr;
  uint16_t latency;
  bool pending;

  portENTER_CRITICAL(&sharedMux);
  desired = desiredMask;
  confirmed = confirmedMask;
  ackAt = lastAcknowledgementAt;
  packets = txPacketCount;
  faults = radioFaultCount;
  battery = currentBatteryPercent;
  rssi = lastRssi;
  snr = lastSnrTenths;
  latency = lastLatencyMs;
  pending = commandPending;
  portEXIT_CRITICAL(&sharedMux);

  pfx::UiTelemetry t;
  t.linkConnected = ackAt != 0 && now - ackAt < pfx::config::LINK_LOST_MS;
  t.rssiDbm = t.linkConnected ? rssi : -127;
  t.snrTenths = t.linkConnected ? snr : INT16_MIN;
  t.linkAgeMs = ackAt == 0 ? UINT32_MAX : now - ackAt;
  t.latencyMs = t.linkConnected ? latency : UINT16_MAX;
  t.batteryPercent = battery;
  t.radioReady = radioReady;
  t.commandConfirmed = t.linkConnected && !pending && confirmed == desired;
  t.packetCount = packets;
  t.faultCount = faults;
  t.failsafeActive = false;
  return t;
}

void IRAM_ATTR onRadioPacket() {
  if (!receiveArmed) return;
  radioPacketFlag = true;
  BaseType_t higherPriorityTaskWoken = pdFALSE;
  if (radioTaskHandle != nullptr) {
    vTaskNotifyGiveFromISR(radioTaskHandle, &higherPriorityTaskWoken);
  }
  if (higherPriorityTaskWoken == pdTRUE) portYIELD_FROM_ISR();
}

bool armReceive() {
  radioPacketFlag = false;
  receiveArmed = true;
  const int16_t state = pfx::radio_support::radio.startReceive();
  if (state == RADIOLIB_ERR_NONE) return true;

  receiveArmed = false;
  portENTER_CRITICAL(&sharedMux);
  ++radioFaultCount;
  portEXIT_CRITICAL(&sharedMux);
  char line[88];
  snprintf(line, sizeof(line), "[RX ARM ERROR] code=%d", static_cast<int>(state));
  logLine(line);
  return false;
}

struct LastTransmission {
  uint16_t sequence = 0;
  uint8_t mask = 0;
  uint32_t generation = 0;
  uint32_t startedAt = 0;
};

LastTransmission lastTransmission;

void processAcknowledgement() {
  if (!radioPacketFlag) return;

  receiveArmed = false;
  radioPacketFlag = false;
  const size_t packetLength = pfx::radio_support::radio.getPacketLength();
  Packet acknowledgement{};
  const int16_t state = pfx::radio_support::radio.readData(
      reinterpret_cast<uint8_t*>(&acknowledgement), sizeof(acknowledgement));

  if (state != RADIOLIB_ERR_NONE || packetLength != sizeof(Packet) ||
      !pfx::protocol::validate(acknowledgement, PacketType::Acknowledgement)) {
    portENTER_CRITICAL(&sharedMux);
    ++radioFaultCount;
    portEXIT_CRITICAL(&sharedMux);
    return;
  }

  const int16_t rssi =
      static_cast<int16_t>(pfx::radio_support::radio.getRSSI());
  const int16_t snr =
      static_cast<int16_t>(pfx::radio_support::radio.getSNR() * 10.0F);
  const uint32_t ackAt = millis();
  const uint16_t latency = static_cast<uint16_t>(
      ackAt - lastTransmission.startedAt > 65534U
          ? 65534U
          : ackAt - lastTransmission.startedAt);
  const uint8_t ackMask = acknowledgement.relayMask & kRelayMask;
  const DesiredSnapshot desired = readDesiredSnapshot();
  const bool confirmsLatest =
      acknowledgement.sequence == lastTransmission.sequence &&
      ackMask == lastTransmission.mask &&
      desired.generation == lastTransmission.generation &&
      desired.mask == lastTransmission.mask;

  portENTER_CRITICAL(&sharedMux);
  confirmedMask = ackMask;
  lastRssi = rssi;
  lastSnrTenths = snr;
  lastAcknowledgementAt = ackAt;
  lastLatencyMs = latency;
  if (confirmsLatest) commandPending = false;
  portEXIT_CRITICAL(&sharedMux);
}

DesiredSnapshot transmitLatest(uint16_t& sequence, bool edgeTransmission,
                               uint32_t& lastPacketLogAt) {
  receiveArmed = false;
  radioPacketFlag = false;
  pfx::radio_support::radio.standby();

  // Re-snapshot after leaving RX mode. A button may have changed while the
  // previous packet was on air; only the newest generation is transmitted.
  const DesiredSnapshot snapshot = readDesiredSnapshot();
  const uint32_t startedAt = millis();
  const Packet control = pfx::protocol::makePacket(
      PacketType::Control, ++sequence, snapshot.mask);
  const int16_t state = pfx::radio_support::radio.transmit(
      reinterpret_cast<const uint8_t*>(&control), sizeof(control));
  const uint32_t finishedAt = millis();

  if (state == RADIOLIB_ERR_NONE) {
    portENTER_CRITICAL(&sharedMux);
    ++txPacketCount;
    portEXIT_CRITICAL(&sharedMux);

    lastTransmission.sequence = sequence;
    lastTransmission.mask = snapshot.mask;
    lastTransmission.generation = snapshot.generation;
    lastTransmission.startedAt = startedAt;

    if (edgeTransmission || finishedAt - lastPacketLogAt >= 1000U) {
      const DesiredSnapshot afterTransmit = readDesiredSnapshot();
      const bool superseded = afterTransmit.generation != snapshot.generation;
      char line[196];
      snprintf(line, sizeof(line),
               "[TX%s%s] seq=%u mask=0x%02X gen=%lu started=%lums finished=%lums detect_to_tx=%lums airtime=%lums",
               edgeTransmission ? " EDGE" : "",
               superseded ? " SUPERSEDED" : "",
               static_cast<unsigned>(sequence),
               static_cast<unsigned>(snapshot.mask),
               static_cast<unsigned long>(snapshot.generation),
               static_cast<unsigned long>(startedAt),
               static_cast<unsigned long>(finishedAt),
               static_cast<unsigned long>(startedAt - snapshot.changedAt),
               static_cast<unsigned long>(finishedAt - startedAt));
      logLine(line);
      lastPacketLogAt = finishedAt;
    }
  } else {
    portENTER_CRITICAL(&sharedMux);
    ++radioFaultCount;
    portEXIT_CRITICAL(&sharedMux);
    char line[104];
    snprintf(line, sizeof(line),
             "[TX ERROR] seq=%u mask=0x%02X code=%d",
             static_cast<unsigned>(sequence),
             static_cast<unsigned>(snapshot.mask), static_cast<int>(state));
    logLine(line);
  }

  // If the command changed during transmit, skip ACK listening and let the
  // next loop send the newest state immediately. Otherwise listen for ACK.
  const DesiredSnapshot current = readDesiredSnapshot();
  if (current.generation == snapshot.generation) armReceive();
  return snapshot;
}

void inputWorker(void*) {
  uint8_t lastMask = readDesiredSnapshot().mask;
  for (;;) {
    const uint8_t nextMask = buttons.update();
    if (nextMask != lastMask) {
      const uint32_t detectedAt = millis();
      const uint8_t oldMask = lastMask;
      publishDesiredMask(nextMask, detectedAt);
      logButtonTransitions(oldMask, nextMask, detectedAt);
      lastMask = nextMask;
    }
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

void radioWorker(void*) {
  pfx::radio_support::radio.setPacketReceivedAction(onRadioPacket);
  armReceive();

  uint16_t sequence = 0;
  uint32_t lastSentAt = 0;
  uint32_t lastSentGeneration = UINT32_MAX;
  uint32_t lastPacketLogAt = 0;
  uint8_t edgeCopiesRemaining = 0;
  uint32_t nextEdgeCopyAt = 0;

  for (;;) {
    const uint32_t now = millis();
    DesiredSnapshot desired = readDesiredSnapshot();

    if (desired.generation != lastSentGeneration) {
      const DesiredSnapshot sent =
          transmitLatest(sequence, true, lastPacketLogAt);
      lastSentGeneration = sent.generation;
      lastSentAt = millis();
      edgeCopiesRemaining = pfx::config::EDGE_EXTRA_COPIES;
      nextEdgeCopyAt = lastSentAt + pfx::config::EDGE_REPEAT_GAP_MS;
      continue;
    }

    // A new edge always outranks an ACK. ACK processing is only performed when
    // there is no unsent generation, so stale radio traffic cannot delay OFF.
    if (radioPacketFlag) {
      processAcknowledgement();
      desired = readDesiredSnapshot();
      if (desired.generation == lastSentGeneration) armReceive();
      continue;
    }

    if (edgeCopiesRemaining > 0 && now >= nextEdgeCopyAt) {
      desired = readDesiredSnapshot();
      if (desired.generation == lastSentGeneration) {
        transmitLatest(sequence, false, lastPacketLogAt);
        lastSentAt = millis();
        --edgeCopiesRemaining;
        nextEdgeCopyAt = lastSentAt + pfx::config::EDGE_REPEAT_GAP_MS;
      }
      continue;
    }

    const uint32_t repeatPeriod = desired.mask
        ? pfx::config::ACTIVE_REPEAT_MS
        : pfx::config::IDLE_REPEAT_MS;
    if (now - lastSentAt >= repeatPeriod) {
      transmitLatest(sequence, false, lastPacketLogAt);
      lastSentAt = millis();
      continue;
    }

    uint32_t waitMs = repeatPeriod - (now - lastSentAt);
    if (edgeCopiesRemaining > 0 && nextEdgeCopyAt > now) {
      waitMs = min(waitMs, nextEdgeCopyAt - now);
    }
    waitMs = constrain(waitMs, static_cast<uint32_t>(1), static_cast<uint32_t>(20));
    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(waitMs));
  }
}

}  // namespace

void setup() {
  // Display/backlight initialization is intentionally first. Previous builds
  // waited on Serial and showed the raw white TFT during that delay.
  ui.begin();
  const uint32_t splashStartedAt = millis();
  ui.drawBoot();
  showBootStage(8, F("STARTING SYSTEM"), 650);

  Serial.begin(115200);
  analogReadResolution(12);
  logQueue = xQueueCreate(32, sizeof(LogMessage));

  Serial.println();
  Serial.println(F("========================================"));
  Serial.println(F("PyrotechnicFX T190 Sender v1.6.3"));
  Serial.println(F("Hardware SPI display + latest-state-wins RF"));
  Serial.println(F("Relay 1 onboard button GPIO21"));
  Serial.println(F("Relays 2-6 external GPIO1,2,3,4,15"));
  Serial.println(F("========================================"));

  showBootStage(24, F("INITIALIZING INPUTS"), 650);
  buttons.begin();
  desiredMask = buttons.update();
  desiredChangedAt = millis();
  showBootStage(42, F("READING BATTERY"), 650);
  currentBatteryPercent = pfx::radio_support::readBatteryPercent();

  showBootStage(64, F("INITIALIZING RADIO"), 750);
  radioReady = pfx::radio_support::begin();
  pfx::radio_support::printRoleAndConfiguration(F("SENDER"));
  showBootStage(radioReady ? 84 : 100, radioReady ? F("STARTING LINK") : F("RADIO FAILED"), 850);

  const uint32_t elapsed = millis() - splashStartedAt;
  if (elapsed < pfx::config::BOOT_SPLASH_MS) {
    delay(pfx::config::BOOT_SPLASH_MS - elapsed);
  }

  if (!radioReady) {
    ui.showFatal(F("SX1262 INIT FAILED"), pfx::radio_support::initCode);
    return;
  }

  showBootStage(100, F("SYSTEM READY"), 900);
  ui.startMain(desiredMask, makeTelemetry(millis()));

  const BaseType_t loggerResult = logQueue != nullptr
      ? xTaskCreatePinnedToCore(
          loggerWorker, "pfx-log", 3072, nullptr, 1, &loggerTaskHandle, 1)
      : pdFAIL;
  const BaseType_t radioResult = xTaskCreatePinnedToCore(
      radioWorker, "pfx-radio", 6144, nullptr, 3, &radioTaskHandle, 0);
  const BaseType_t inputResult = xTaskCreatePinnedToCore(
      inputWorker, "pfx-input", 3072, nullptr, 4, &inputTaskHandle, 1);

  if (loggerResult != pdPASS || radioResult != pdPASS || inputResult != pdPASS) {
    radioReady = false;
    ui.showFatal(F("TASK START FAILED"),
                 loggerResult != pdPASS ? static_cast<int>(loggerResult)
                 : (radioResult != pdPASS ? static_cast<int>(radioResult)
                                          : static_cast<int>(inputResult)));
    Serial.println(F("[FATAL] unable to create logger, radio, or input task"));
    return;
  }

  logLine("[READY] input scanner 1ms; RELEASE first HIGH; RF latest state wins");
}

void loop() {
  if (!radioReady || radioTaskHandle == nullptr || inputTaskHandle == nullptr) {
    delay(100);
    return;
  }

  static uint32_t displayedGeneration = UINT32_MAX;
  const DesiredSnapshot desired = readDesiredSnapshot();
  if (desired.generation != displayedGeneration) {
    ui.setRelayMaskImmediate(desired.mask);
    displayedGeneration = desired.generation;
  }

  const uint32_t now = millis();
  static uint32_t lastBatteryReadAt = 0;
  if (now - lastBatteryReadAt >= 3000U) {
    const int battery = pfx::radio_support::readBatteryPercent();
    portENTER_CRITICAL(&sharedMux);
    currentBatteryPercent = battery;
    portEXIT_CRITICAL(&sharedMux);
    lastBatteryReadAt = now;
  }

  static uint32_t lastUiRefreshAt = 0;
  if (now - lastUiRefreshAt >= 75U) {
    ui.update(desired.mask, makeTelemetry(now));
    lastUiRefreshAt = now;
  }

  delay(1);
}
