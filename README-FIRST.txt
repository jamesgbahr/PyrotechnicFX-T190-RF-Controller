PYROTECHNICFX T190 6-RELAY TEST RELEASE v1.6.3
================================================

This package contains two independent PlatformIO projects:

  PFX-TX  = six-channel transmitter
  PFX-RX  = six-channel receiver

WHAT v1.6.3 CHANGES
-------------------
v1.6.3 is built directly over the v1.5.8 hardware-SPI, full-frame display,
1 ms input-scanning, latest-state-wins RF, and asynchronous ACK architecture.
The RF, input, failsafe, and receiver output ordering remain unchanged.

Approved visual changes:
  - Restores the molten ember/cracked-fire boot background.
  - Uses the approved metallic PyrotechnicFX wordmark.
  - Provides separate TRANSMITTER and RECEIVER boot labels.
  - Contains no gray progress box or overlay across the boot branding.
  - Restores the clean orange flame for every active relay channel.
  - Regenerates all channel cards as exact native 52 x 64 assets.
  - Keeps telemetry, transport values, active state, and relay states out of the
    static main-screen chrome so values cannot be duplicated.
  - Redraws column separators whenever a dynamic region is refreshed.

DISPLAY BASE PRESERVED FROM v1.5.8
---------------------------------
  - Backlight OFF before TFT initialization.
  - Separate 40 MHz hardware-SPI display bus.
  - Full boot bitmap transferred before the backlight turns on.
  - Complete first main frame composed while the backlight is off.
  - Exact 170 x 320 full-screen assets and 52 x 64 relay assets.

BUTTON AND RF BASE PRESERVED FROM v1.5.8
----------------------------------------
  - Button scanning every 1 ms on a dedicated high-priority task.
  - PRESS requires one stable millisecond.
  - RELEASE is accepted on the first HIGH sample.
  - New button generations replace older command generations immediately.
  - No blocking 40 ms transmitter ACK wait.
  - ACK reception is asynchronous and used for telemetry.
  - Receiver applies output state before ACK, logging, or UI work.

WINDOWS QUICK START
-------------------
1. Extract the ZIP completely into a new folder.
2. Do not copy it over v1.5.8 or an older release.
3. Close every serial monitor.
4. Connect only the receiver and run 2_FLASH_RECEIVER.bat.
5. Disconnect RX, connect TX, and run 1_FLASH_TRANSMITTER.bat.
6. Reconnect both units and test with LEDs or a logic tester.

The scripts build, erase, and upload the complete image. Do not write only
firmware.bin to address 0x0000.

TRANSMITTER INPUTS
------------------
Relay 1: onboard USER button, GPIO21
Relay 2: GPIO1 to GND
Relay 3: GPIO2 to GND
Relay 4: GPIO3 to GND
Relay 5: GPIO4 to GND
Relay 6: GPIO15 to GND

RECEIVER OUTPUTS
----------------
Relay 1: GPIO1
Relay 2: GPIO2
Relay 3: GPIO3
Relay 4: GPIO4
Relay 5: GPIO15
Relay 6: GPIO16

Use transistor/MOSFET or opto-isolated drivers and a separate relay supply.
Never drive a relay or solenoid coil directly from a T190 GPIO pin.

SAFETY
------
Test with LEDs or a logic analyzer. Keep relay loads, igniters, valves, fuel,
and flame-effect hardware disconnected until every control and failsafe case is
verified repeatedly. This prototype is not a certified firing or life-safety
controller. Use an independent physical E-stop and fuel shutoff.
