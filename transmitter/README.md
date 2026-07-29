# PyrotechnicFX T190 TRANSMITTER v1.6.3

Six-channel momentary transmitter for the Heltec Vision Master T190.

## v1.6.3 behavior

- 1 ms high-priority input scan
- 1 ms PRESS qualification
- first-HIGH RELEASE/OFF
- latest-state-wins radio scheduling
- no blocking ACK wait in the command path
- asynchronous ACK telemetry
- hardware-HSPI 40 MHz TFT rendering
- clean two-second boot splash
- exact 52 x 64 relay cards

## Inputs

- CH1: onboard USER button, GPIO21
- CH2-CH6: GPIO1, GPIO2, GPIO3, GPIO4, GPIO15 to GND

Use `FULL_FLASH_TRANSMITTER.bat` for a complete build, erase, and upload.
