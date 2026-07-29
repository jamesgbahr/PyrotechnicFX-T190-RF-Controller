#pragma once
#include <Arduino.h>
#include <RadioLib.h>

namespace pfx::radio_support {

extern SX1262 radio;
extern int16_t initCode;

bool begin();
float readBatteryVoltage();
int readBatteryPercent();
void printRoleAndConfiguration(const __FlashStringHelper* roleName);

}  // namespace pfx::radio_support
