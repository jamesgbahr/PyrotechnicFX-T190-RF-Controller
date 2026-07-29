#!/usr/bin/env python3
"""Validate v1.6.3 native PNG assets and rebuild RGB565 C++ headers."""
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / "assets"
GENERATED = ROOT / "include" / "generated"
ROLE_TX = "TX" in ROOT.name.upper()

BOOT = ASSETS / ("boot_sender_170x320.png" if ROLE_TX else "boot_receiver_170x320.png")
CHROME = ASSETS / ("main_sender_chrome_170x320.png" if ROLE_TX else "main_receiver_chrome_170x320.png")


def rgb565(pixel):
    r, g, b = pixel[:3]
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def load_exact(path: Path, size: tuple[int, int]) -> Image.Image:
    image = Image.open(path).convert("RGB")
    if image.size != size:
        raise SystemExit(f"{path.name}: expected {size}, found {image.size}")
    return image


def write_header(items, path: Path):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as handle:
        handle.write("#pragma once\n#include <Arduino.h>\n\n")
        for symbol, image in items:
            values = [rgb565(pixel) for pixel in image.getdata()]
            handle.write(f"constexpr uint16_t {symbol}_WIDTH = {image.width};\n")
            handle.write(f"constexpr uint16_t {symbol}_HEIGHT = {image.height};\n")
            handle.write(f"const uint16_t {symbol}[] PROGMEM = {{\n")
            for offset in range(0, len(values), 12):
                chunk = ", ".join(f"0x{value:04X}" for value in values[offset:offset + 12])
                handle.write(f"  {chunk},\n")
            handle.write("};\n\n")


def main():
    boot = load_exact(BOOT, (170, 320))
    chrome = load_exact(CHROME, (170, 320))
    cards = []
    for channel in range(1, 7):
        off = load_exact(ASSETS / f"relay_{channel}_off_52x64.png", (52, 64))
        on = load_exact(ASSETS / f"relay_{channel}_on_52x64.png", (52, 64))
        cards.extend([
            (f"PFX_RELAY_{channel}_OFF", off),
            (f"PFX_RELAY_{channel}_ON", on),
        ])

    write_header([("PFX_BOOT_SCREEN", boot)], GENERATED / "boot_screen_rgb565.h")
    write_header([("PFX_MAIN_CHROME", chrome)], GENERATED / "main_chrome_rgb565.h")
    write_header(cards, GENERATED / "relay_cards_rgb565.h")
    print(f"Validated and regenerated {'TX' if ROLE_TX else 'RX'} v1.6.3 graphics.")


if __name__ == "__main__":
    main()
