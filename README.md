# PyrotechnicFX T190 RF Relay Controller

A custom six-channel LoRa relay-control system for a pair of **Heltec Vision Master T190** devices.

One T190 operates as a handheld transmitter and the other operates as a receiver that controls six external relay inputs. The project includes separate transmitter and receiver firmware, a custom 170×320 display interface, live RF telemetry, immediate momentary control, receiver acknowledgments, communication-loss failsafes, Windows build-and-flash scripts, and detailed serial diagnostics.

**Current confirmed stable baseline: v1.6.3**

---

## Description

The PyrotechnicFX T190 RF Relay Controller is a purpose-built wireless control platform for experimental live-event, automation, special-effects, and remote-switching applications.

The system uses the onboard ESP32-S3, SX1262 LoRa radio, and 170×320 display found on the Heltec Vision Master T190. It is divided into two independent PlatformIO projects:

- **Transmitter** — reads momentary control inputs, sends the latest relay state, displays local channel activity, and shows receiver acknowledgments and RF telemetry.
- **Receiver** — validates control packets, updates six relay-control GPIO outputs, returns acknowledgments, reports link quality, and forces outputs into a safe OFF state when communication is lost.

The firmware is designed around a **latest-state-wins** model so a newly detected button press or release replaces older pending states instead of waiting behind stale packet bursts.

---

## Main Capabilities

### Six independent momentary channels

The transmitter supports six control channels.

A channel remains active only while its corresponding button is held. Releasing the button publishes the OFF state immediately.

### LoRa communication

The two T190 devices communicate using the onboard SX1262 LoRa radio.

Default configuration:

```text
Frequency: 915 MHz
Channels: 6
Control mode: Momentary
Architecture: Paired transmitter and receiver
```

Both devices must use matching RF and protocol settings.

### Immediate input response

Button input runs independently from display rendering and serial output.

The firmware supports:

- quick taps
- long holds
- repeated presses
- immediate releases
- latest-state-wins transmission
- local relay-card feedback

### Receiver acknowledgments

The receiver sends an acknowledgment after processing a valid control packet.

The transmitter can use these acknowledgments to display:

- link state
- RSSI
- SNR
- packet age
- acknowledgment status
- communication latency
- retry or fault information

### Live telemetry

The custom interface can display:

- RF link status
- RSSI
- SNR
- packet age
- battery or USB-power state
- packet count
- ACK state
- error count
- uptime
- active channel
- latency

### Custom PyrotechnicFX interface

The transmitter and receiver use native 170×320 display graphics, including:

- PyrotechnicFX branded boot screen
- live boot progress
- transmitter or receiver role identification
- telemetry dashboard
- six relay cards
- clean flame graphic for active channels
- active-channel status strip
- packet and uptime information

### Hidden screen transitions

The firmware prepares boot and dashboard graphics before revealing them with the backlight.

This avoids visible section-by-section screen construction.

### Receiver failsafe

If valid control packets stop arriving, the receiver forces all relay outputs into the configured safe OFF state.

The software failsafe is an additional layer and must not replace physical safety systems.

### Watchdog-safe receiver

The v1.6.3 receiver uses cooperative task scheduling so the RF worker does not starve CPU 0 and trigger the ESP32-S3 task watchdog.

Expected receiver startup marker:

```text
PyrotechnicFX T190 Receiver v1.6.3
Cooperative RF worker + watchdog-safe CPU0 scheduling
[READY] watchdog-safe RX worker; waiting for authenticated control packets
```

### Serial diagnostics

Both projects provide detailed serial output at **115200 baud**.

Diagnostics include:

- radio initialization
- button detection
- relay masks
- transmitted packet sequences
- acknowledgments
- RSSI and SNR
- output changes
- failsafe events
- errors
- watchdog-safe startup confirmation

---

## Repository Structure

```text
PyrotechnicFX-T190-RF-Controller/
├── 1_FLASH_TRANSMITTER.bat
├── 2_FLASH_RECEIVER.bat
├── 3_BUILD_TRANSMITTER_ONLY.bat
├── 4_BUILD_RECEIVER_ONLY.bat
├── README.md
├── README-FIRST.txt
├── LICENSE
├── SERIAL-DIAGNOSTICS.txt
├── APPROVED-UI-170x320.png
├── RELEASE-NOTES-v1.6.3.md
├── transmitter/
└── receiver/
```

The transmitter and receiver are separate PlatformIO projects.

---

## Required Hardware

- 2× Heltec Vision Master T190
- 2× suitable 915 MHz antennas
- USB data cables
- momentary transmitter buttons
- external relay or opto-isolated input board
- separate regulated relay-board power supply
- wiring and enclosure hardware
- LEDs or logic analyzer for initial bench testing

---

## Receiver GPIO Outputs

| Channel | GPIO |
|---|---:|
| Relay 1 | GPIO 1 |
| Relay 2 | GPIO 2 |
| Relay 3 | GPIO 3 |
| Relay 4 | GPIO 4 |
| Relay 5 | GPIO 15 |
| Relay 6 | GPIO 16 |

The GPIO pins are control signals only.

Do not power relay coils directly from a T190 GPIO or from the T190 3.3 V output.

---

## Relay-Board Power

Power the relay board from a separate regulated supply appropriate for the selected module.

Typical connection:

```text
T190 GPIO        → Relay-board input
T190 GND         → Relay-board control ground
External supply  → Relay-board power input
```

Confirm that the relay-board inputs accept 3.3 V logic.

Some relay boards may require:

- transistor drivers
- MOSFET drivers
- logic-level conversion
- opto-isolated input wiring
- flyback protection

---

# Easy Windows Installation

The repository includes Windows batch files that automate dependency setup, compilation, device erasing, firmware upload, and optional serial monitoring.

## 1. Download the project

Clone the repository:

```bash
git clone https://github.com/jamesgbahr/PyrotechnicFX-T190-RF-Controller.git
cd PyrotechnicFX-T190-RF-Controller
```

Or use:

```text
GitHub → Code → Download ZIP
```

Extract the ZIP before running the batch files.

## 2. Flash the receiver first

1. Connect the receiver T190 using a USB data cable.
2. Close any serial monitor already using the COM port.
3. Double-click:

```text
2_FLASH_RECEIVER.bat
```

4. Follow the prompts.
5. Leave the receiver powered after flashing.

## 3. Flash the transmitter

1. Connect the transmitter T190.
2. Double-click:

```text
1_FLASH_TRANSMITTER.bat
```

3. Follow the prompts.

The receiver should be flashed and powered before testing the transmitter.

## 4. Build without flashing

To compile without uploading:

```text
3_BUILD_TRANSMITTER_ONLY.bat
4_BUILD_RECEIVER_ONLY.bat
```

## Project-specific Windows scripts

The individual projects also contain:

```text
transmitter/FULL_FLASH_TRANSMITTER.bat
receiver/FULL_FLASH_RECEIVER.bat
```

---

# Manual PlatformIO Installation

## Software requirements

Install:

- Visual Studio Code
- PlatformIO extension
- Git for Windows
- compatible USB serial drivers

## Build the receiver

Open the `receiver` folder directly in Visual Studio Code.

```bash
cd receiver
pio run
```

## Flash the receiver

```bash
pio run -t erase
pio run -t upload
```

Leave the receiver powered after flashing.

## Build and flash the transmitter

Open the `transmitter` folder directly in Visual Studio Code.

```bash
cd transmitter
pio run
pio run -t erase
pio run -t upload
```

---

## Serial Monitor

Open at 115200 baud:

```bash
pio device monitor -b 115200
```

Expected stable receiver startup:

```text
PyrotechnicFX T190 Receiver v1.6.3
Cooperative RF worker + watchdog-safe CPU0 scheduling
[READY] watchdog-safe RX worker; waiting for authenticated control packets
```

---

## Basic Test Procedure

1. Disconnect relays, valves, igniters, fuel, and effect hardware.
2. Connect LEDs or a logic analyzer to the receiver outputs.
3. Flash and power the receiver.
4. Confirm the v1.6.3 watchdog-safe startup message.
5. Flash and power the transmitter.
6. Confirm the transmitter changes from `WAIT` to `LIVE`.
7. Confirm RSSI and SNR appear after a valid ACK exchange.
8. Test every channel using quick taps.
9. Test every channel using long holds.
10. Confirm each output turns off immediately after release.
11. Turn off the transmitter and confirm the receiver failsafe turns all outputs off.

---

## Troubleshooting

### Transmitter remains on WAIT

Check:

- receiver power
- antenna connection
- matching RF frequency
- matching firmware versions
- receiver serial output
- radio initialization status

### RSSI and SNR remain blank

RSSI and SNR appear only after a valid receiver acknowledgment.

Blank telemetry generally means the transmitter has not completed a valid control-and-ACK exchange with the receiver.

### Receiver repeatedly reboots

Confirm that the receiver is running v1.6.3 or newer.

Expected startup text:

```text
Cooperative RF worker + watchdog-safe CPU0 scheduling
```

Older receiver firmware could starve the CPU 0 idle task and trigger the watchdog.

### Relay board does not trigger

Check:

- active-HIGH versus active-LOW configuration
- 3.3 V logic compatibility
- common ground
- external relay-board power
- GPIO assignment
- optocoupler jumper configuration

### Batch file cannot find PlatformIO

Install the PlatformIO extension in Visual Studio Code, then reopen the terminal.

The included scripts also attempt to repair common Python and PlatformIO dependency issues.

---

## Planned Expansion

Possible future additions include:

- eight-channel receiver support
- configurable GPIO mapping
- channel naming
- device pairing
- configurable LoRa frequency
- adjustable failsafe timeout
- encrypted configuration storage
- 3D-printable enclosure files
- XLR panel layouts
- relay-board mounting templates
- battery calibration
- firmware update tools
- additional telemetry pages

---

## Safety

This is experimental control firmware.

Bench-test with LEDs or a logic analyzer before connecting:

- relays
- solenoids
- valves
- igniters
- firing systems
- fuel systems
- flame effects
- pyrotechnic equipment

Use independent physical safety systems, including:

- hardwired emergency stop
- master output disable
- fuel shutoff
- appropriate fusing
- electrical isolation
- qualified operators
- applicable permits and inspections

Software and wireless communication must never be the only safety layer.

---

## License

Copyright © PyrotechnicFX.

See `LICENSE` for the repository usage terms.
