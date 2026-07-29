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
constexpr uint32_t kAckTurnaroundUs = 900;

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
QueueHandle_t logQueue = nullptr;
TaskHandle_t loggerTaskHandle = nullptr;
portMUX_TYPE sharedMux = portMUX_INITIALIZER_UNLOCKED;

volatile bool radioPacketFlag = false;
volatile bool receiveArmed = false;

uint8_t actualRelayMask = 0;
uint32_t relayGeneration = 0;
uint32_t lastValidCommandAt = 0;
uint32_t rxPacketCount = 0;
uint32_t packetErrorCount = 0;
int currentBatteryPercent = -1;
int16_t lastRssi = -127;
int16_t lastSnrTenths = INT16_MIN;
bool haveReceivedCommand = false;
bool haveSequence = false;
bool failsafeActive = false;
uint16_t lastSequence = 0;

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

void writeRelay(uint8_t index, bool active) {
  const uint8_t pin = pfx::pins::RECEIVER_RELAYS[index];
  const uint8_t level = pfx::config::RELAY_ACTIVE_LOW
      ? (active ? LOW : HIGH)
      : (active ? HIGH : LOW);
  digitalWrite(pin, level);
}

void applyRelayMask(uint8_t relayMask) {
  relayMask &= kRelayMask;
  for (uint8_t i = 0; i < kRelayCount; ++i) {
    writeRelay(i, ((relayMask >> i) & 1U) != 0);
  }

  portENTER_CRITICAL(&sharedMux);
  if (actualRelayMask != relayMask) {
    actualRelayMask = relayMask;
    ++relayGeneration;
  }
  portEXIT_CRITICAL(&sharedMux);
}

void configureSafeOutputs() {
  for (uint8_t i = 0; i < kRelayCount; ++i) {
    const uint8_t pin = pfx::pins::RECEIVER_RELAYS[i];
    digitalWrite(pin, pfx::config::RELAY_ACTIVE_LOW ? HIGH : LOW);
    pinMode(pin, OUTPUT);
  }
  applyRelayMask(0);
}

struct ReceiverSnapshot {
  uint8_t mask;
  uint32_t generation;
};

ReceiverSnapshot readReceiverSnapshot() {
  ReceiverSnapshot snapshot{};
  portENTER_CRITICAL(&sharedMux);
  snapshot.mask = actualRelayMask;
  snapshot.generation = relayGeneration;
  portEXIT_CRITICAL(&sharedMux);
  return snapshot;
}

pfx::UiTelemetry makeTelemetry(uint32_t now) {
  uint8_t mask;
  uint32_t lastCommand;
  uint32_t packets;
  uint32_t errors;
  int battery;
  int16_t rssi;
  int16_t snr;
  bool haveCommand;
  bool failsafe;

  portENTER_CRITICAL(&sharedMux);
  mask = actualRelayMask;
  (void)mask;
  lastCommand = lastValidCommandAt;
  packets = rxPacketCount;
  errors = packetErrorCount;
  battery = currentBatteryPercent;
  rssi = lastRssi;
  snr = lastSnrTenths;
  haveCommand = haveReceivedCommand;
  failsafe = failsafeActive;
  portEXIT_CRITICAL(&sharedMux);

  pfx::UiTelemetry t;
  t.linkConnected = haveCommand &&
      now - lastCommand < pfx::config::LINK_LOST_MS;
  t.rssiDbm = t.linkConnected ? rssi : -127;
  t.snrTenths = t.linkConnected ? snr : INT16_MIN;
  t.linkAgeMs = haveCommand ? now - lastCommand : UINT32_MAX;
  t.latencyMs = UINT16_MAX;
  t.batteryPercent = battery;
  t.radioReady = radioReady;
  t.commandConfirmed = true;
  t.packetCount = packets;
  t.faultCount = errors;
  t.failsafeActive = failsafe;
  return t;
}

void processControlPacket(const Packet& control) {
  const uint32_t receivedAt = millis();

  bool linkHadTimedOut;
  bool sequenceIsNew;
  bool sequenceIsDuplicate;
  uint8_t appliedMask;
  int16_t rssi;
  int16_t snr;

  portENTER_CRITICAL(&sharedMux);
  linkHadTimedOut = !haveReceivedCommand ||
      receivedAt - lastValidCommandAt > pfx::config::RECEIVER_FAILSAFE_MS;
  sequenceIsNew = !haveSequence || linkHadTimedOut ||
      static_cast<int16_t>(control.sequence - lastSequence) > 0;
  sequenceIsDuplicate = haveSequence && control.sequence == lastSequence;
  portEXIT_CRITICAL(&sharedMux);

  if (!sequenceIsNew && !sequenceIsDuplicate) return;

  // RF metrics are captured before the radio is switched to TX for the ACK.
  rssi = static_cast<int16_t>(pfx::radio_support::radio.getRSSI());
  snr = static_cast<int16_t>(pfx::radio_support::radio.getSNR() * 10.0F);

  if (sequenceIsNew) {
    // Both ON and OFF are applied before any logging, UI work, or ACK handling.
    // RELEASE therefore reaches the output on the first valid newest packet.
    applyRelayMask(control.relayMask);
  }

  portENTER_CRITICAL(&sharedMux);
  ++rxPacketCount;
  lastValidCommandAt = receivedAt;
  haveReceivedCommand = true;
  failsafeActive = false;
  lastRssi = rssi;
  lastSnrTenths = snr;
  if (sequenceIsNew) {
    lastSequence = control.sequence;
    haveSequence = true;
  }
  appliedMask = actualRelayMask;
  portEXIT_CRITICAL(&sharedMux);

  // Give the sender enough turnaround time to leave TX and enter continuous RX.
  // This delay is after the relay output update, so it cannot delay OFF.
  delayMicroseconds(kAckTurnaroundUs);
  const Packet acknowledgement = pfx::protocol::makePacket(
      PacketType::Acknowledgement, control.sequence, appliedMask);
  const int16_t ackState = pfx::radio_support::radio.transmit(
      reinterpret_cast<const uint8_t*>(&acknowledgement),
      sizeof(acknowledgement));

  if (ackState != RADIOLIB_ERR_NONE) {
    portENTER_CRITICAL(&sharedMux);
    ++packetErrorCount;
    portEXIT_CRITICAL(&sharedMux);
    char line[104];
    snprintf(line, sizeof(line),
             "[ACK TX ERROR] seq=%u mask=0x%02X code=%d",
             static_cast<unsigned>(control.sequence),
             static_cast<unsigned>(appliedMask),
             static_cast<int>(ackState));
    logLine(line);
  }

  static uint32_t lastPacketLogAt = 0;
  const uint32_t now = millis();
  if (sequenceIsNew || linkHadTimedOut || now - lastPacketLogAt >= 1000U) {
    char line[144];
    snprintf(line, sizeof(line),
             "[RX] seq=%u requested=0x%02X applied=0x%02X rssi=%d snr=%d.%ddB ack=%s",
             static_cast<unsigned>(control.sequence),
             static_cast<unsigned>(control.relayMask & kRelayMask),
             static_cast<unsigned>(appliedMask),
             static_cast<int>(rssi), static_cast<int>(snr / 10),
             static_cast<int>(abs(snr % 10)),
             ackState == RADIOLIB_ERR_NONE ? "SENT" : "FAILED");
    logLine(line);
    lastPacketLogAt = now;
  }
}

void enforceFailsafe(uint32_t now) {
  bool shouldTrip = false;
  portENTER_CRITICAL(&sharedMux);
  shouldTrip = haveReceivedCommand &&
      now - lastValidCommandAt > pfx::config::RECEIVER_FAILSAFE_MS &&
      !failsafeActive;
  portEXIT_CRITICAL(&sharedMux);

  if (!shouldTrip) return;

  applyRelayMask(0);
  portENTER_CRITICAL(&sharedMux);
  failsafeActive = true;
  portEXIT_CRITICAL(&sharedMux);
  logLine("[FAILSAFE] communication timeout: all relay outputs forced OFF");
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
  ++packetErrorCount;
  portEXIT_CRITICAL(&sharedMux);
  char line[88];
  snprintf(line, sizeof(line), "[RX ARM ERROR] code=%d", static_cast<int>(state));
  logLine(line);
  return false;
}

void processReceivedPacket() {
  if (!radioPacketFlag) return;

  receiveArmed = false;
  radioPacketFlag = false;
  const size_t packetLength = pfx::radio_support::radio.getPacketLength();
  Packet control{};
  const int16_t state = pfx::radio_support::radio.readData(
      reinterpret_cast<uint8_t*>(&control), sizeof(control));

  if (state != RADIOLIB_ERR_NONE || packetLength != sizeof(Packet)) {
    portENTER_CRITICAL(&sharedMux);
    ++packetErrorCount;
    portEXIT_CRITICAL(&sharedMux);
    char line[104];
    snprintf(line, sizeof(line),
             "[RX READ ERROR] code=%d length=%u expected=%u",
             static_cast<int>(state), static_cast<unsigned>(packetLength),
             static_cast<unsigned>(sizeof(Packet)));
    logLine(line);
    return;
  }

  if (!pfx::protocol::validate(control, PacketType::Control)) {
    portENTER_CRITICAL(&sharedMux);
    ++packetErrorCount;
    portEXIT_CRITICAL(&sharedMux);
    logLine("[RX INVALID] packet authentication or CRC failed");
    return;
  }

  processControlPacket(control);
}

void radioWorker(void*) {
  // Interrupt-driven receive prevents the former tight blocking receive loop
  // from starving the ESP32-S3 Core 0 idle task and triggering the watchdog.
  pfx::radio_support::radio.setPacketReceivedAction(onRadioPacket);
  if (!armReceive()) vTaskDelay(pdMS_TO_TICKS(20));
  logLine("[RX TASK] interrupt-driven receive active");

  for (;;) {
    // Block/yield until DIO1 signals a packet, with a short timeout so the
    // failsafe remains responsive even when no RF traffic is present.
    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(25));

    if (radioPacketFlag) processReceivedPacket();
    enforceFailsafe(millis());

    // readData() or ACK transmission leaves RX mode. Re-arm continuous RX
    // after all packet processing; retry gently on transient SPI/radio errors.
    if (!receiveArmed && !armReceive()) {
      vTaskDelay(pdMS_TO_TICKS(10));
    }

    // Explicitly yield Core 0 so its idle task and system housekeeping always
    // run, even during continuous packet traffic.
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

}  // namespace

void setup() {
  // Outputs are made safe before display, Serial, radio, or any delay.
  configureSafeOutputs();
  ui.begin();
  const uint32_t splashStartedAt = millis();
  ui.drawBoot();
  showBootStage(8, F("STARTING SYSTEM"), 650);

  Serial.begin(115200);
  analogReadResolution(12);
  logQueue = xQueueCreate(32, sizeof(LogMessage));

  Serial.println();
  Serial.println(F("========================================"));
  Serial.println(F("PyrotechnicFX T190 Receiver v1.6.3"));
  Serial.println(F("Interrupt-driven RF/output worker isolated from display rendering"));
  Serial.println(F("Relay outputs GPIO1,2,3,4,15,16"));
  Serial.println(F("========================================"));

  showBootStage(26, F("SAFE OUTPUT CHECK"), 650);
  showBootStage(44, F("READING BATTERY"), 650);
  currentBatteryPercent = pfx::radio_support::readBatteryPercent();
  showBootStage(64, F("INITIALIZING RADIO"), 750);
  radioReady = pfx::radio_support::begin();
  pfx::radio_support::printRoleAndConfiguration(F("RECEIVER"));
  showBootStage(radioReady ? 84 : 100, radioReady ? F("ARMING RECEIVER") : F("RADIO FAILED"), 850);

  const uint32_t elapsed = millis() - splashStartedAt;
  if (elapsed < pfx::config::BOOT_SPLASH_MS) {
    delay(pfx::config::BOOT_SPLASH_MS - elapsed);
  }

  if (!radioReady) {
    applyRelayMask(0);
    ui.showFatal(F("SX1262 INIT FAILED"), pfx::radio_support::initCode);
    return;
  }

  showBootStage(100, F("SYSTEM READY"), 900);
  ui.startMain(0, makeTelemetry(millis()));
  const BaseType_t loggerResult = logQueue != nullptr
      ? xTaskCreatePinnedToCore(
          loggerWorker, "pfx-log", 3072, nullptr, 1, &loggerTaskHandle, 1)
      : pdFAIL;
  const BaseType_t taskResult = xTaskCreatePinnedToCore(
      radioWorker, "pfx-rx-radio", 6144, nullptr, 4,
      &radioTaskHandle, 0);
  if (loggerResult != pdPASS || taskResult != pdPASS) {
    radioReady = false;
    applyRelayMask(0);
    ui.showFatal(F("TASK START FAILED"),
                 loggerResult != pdPASS ? static_cast<int>(loggerResult)
                                        : static_cast<int>(taskResult));
    Serial.println(F("[FATAL] unable to create receiver worker task"));
    return;
  }

  logLine("[READY] waiting for authenticated control packets");
}

void loop() {
  if (!radioReady || radioTaskHandle == nullptr) {
    applyRelayMask(0);
    delay(100);
    return;
  }

  static uint32_t displayedGeneration = UINT32_MAX;
  const ReceiverSnapshot receiver = readReceiverSnapshot();
  if (receiver.generation != displayedGeneration) {
    ui.setRelayMaskImmediate(receiver.mask);
    displayedGeneration = receiver.generation;
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
    ui.update(receiver.mask, makeTelemetry(now));
    lastUiRefreshAt = now;
  }

  delay(1);
}
