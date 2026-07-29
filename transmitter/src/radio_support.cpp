#include "radio_support.h"

#include <SPI.h>
#include "board_pins.h"
#include "config.h"

namespace pfx::radio_support {

SX1262 radio = new Module(pins::LORA_NSS,
                           pins::LORA_DIO1,
                           pins::LORA_RST,
                           pins::LORA_BUSY);
int16_t initCode = RADIOLIB_ERR_UNKNOWN;

bool begin() {
  SPI.begin(pins::LORA_SCK, pins::LORA_MISO, pins::LORA_MOSI, pins::LORA_NSS);

  // The RA62 variants seen on T190 boards normally use a TCXO. Try the
  // configured value first, then safe fallbacks for module variants.
  const float tcxoCandidates[] = {config::RF_TCXO_VOLTAGE, 1.6F, 0.0F};
  for (uint8_t attempt = 0; attempt < 3; ++attempt) {
    if (attempt > 0) {
      radio.reset();
      delay(12);
    }
    initCode = radio.begin(config::RF_FREQUENCY_MHZ,
                           config::RF_BANDWIDTH_KHZ,
                           config::RF_SPREADING_FACTOR,
                           config::RF_CODING_RATE,
                           RADIOLIB_SX126X_SYNC_WORD_PRIVATE,
                           config::RF_OUTPUT_POWER_DBM,
                           config::RF_PREAMBLE_SYMBOLS,
                           tcxoCandidates[attempt],
                           false);
    if (initCode == RADIOLIB_ERR_NONE) break;
  }

  if (initCode != RADIOLIB_ERR_NONE) return false;

  radio.setDio2AsRfSwitch(true);
  radio.setCRC(2);
  radio.explicitHeader();
  radio.setCurrentLimit(100.0F);
  return true;
}

float readBatteryVoltage() {
  pinMode(pins::BATTERY_ADC_ENABLE, OUTPUT);
  digitalWrite(pins::BATTERY_ADC_ENABLE, HIGH);
  delay(3);
  const uint32_t millivolts = analogReadMilliVolts(pins::BATTERY_ADC);
  digitalWrite(pins::BATTERY_ADC_ENABLE, LOW);
  return (static_cast<float>(millivolts) * 4.9F) / 1000.0F;
}

int readBatteryPercent() {
  const float voltage = readBatteryVoltage();
  if (voltage < 2.0F || voltage > 4.5F) return -1;
  const float ratio = (voltage - config::BATTERY_EMPTY_V) /
                      (config::BATTERY_FULL_V - config::BATTERY_EMPTY_V);
  return constrain(static_cast<int>(ratio * 100.0F + 0.5F), 0, 100);
}

void printRoleAndConfiguration(const __FlashStringHelper* roleName) {
  Serial.println();
  Serial.println(F("PyrotechnicFX T190 6-Relay RF Controller"));
  Serial.print(F("Role: "));
  Serial.println(roleName);
  Serial.print(F("Frequency: "));
  Serial.print(config::RF_FREQUENCY_MHZ, 3);
  Serial.println(F(" MHz"));
  Serial.print(F("Radio init code: "));
  Serial.println(initCode);
}

}  // namespace pfx::radio_support
