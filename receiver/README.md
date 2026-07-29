# PyrotechnicFX T190 RECEIVER v1.6.3

Six-channel receiver for the Heltec Vision Master T190.

## v1.6.3 behavior

- receiver RF/output worker isolated from display drawing
- newest authenticated sequence applied immediately
- OFF applied before ACK, Serial logging, or UI rendering
- duplicate packets acknowledged without re-toggling outputs
- 500 ms communication-loss failsafe
- hardware-HSPI 40 MHz TFT rendering
- clean two-second boot splash
- exact 52 x 64 relay cards

## Outputs

- CH1-CH6: GPIO1, GPIO2, GPIO3, GPIO4, GPIO15, GPIO16

Use proper MOSFET/transistor or opto-isolated drivers. Use
`FULL_FLASH_RECEIVER.bat` for a complete build, erase, and upload.
