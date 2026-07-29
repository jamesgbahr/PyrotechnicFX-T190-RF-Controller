# Graphics - v1.6.3

Native T190 dimensions: 170 x 320 pixels.

## Runtime assets

- Boot: 170 x 320
- Main chrome: 170 x 320
- Relay cards: exactly 52 x 64
- Card positions: x = 5, 60, 115; y = 165, 232

The main chrome contains labels, borders, and the locked PyrotechnicFX wordmark,
but it contains no sample telemetry and no relay state. Dynamic values and cards
are rendered by firmware.

The TFT runs on the ESP32-S3 hardware HSPI host at 40 MHz. Full-screen and card
bitmaps are transferred with setAddrWindow() + writePixels().

Run this to validate dimensions and rebuild RGB565 headers:

    python tools/regenerate_graphics.py

The generator stops if a PNG has the wrong dimensions. ui.cpp also contains
compile-time dimension checks, preventing a mismatched card array from being
read past its boundary.
