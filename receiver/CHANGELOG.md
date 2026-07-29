# Changelog

## v1.6.3
- Restored the approved molten ember boot background and metallic wordmark.
- Added separate TRANSMITTER and RECEIVER boot role labels.
- Restored a clean orange flame graphic for all active relay channels.
- Regenerated native 170 x 320 boot/main assets and native 52 x 64 relay cards.
- Kept sample telemetry and relay states out of the static chrome.
- Redraws status, transport, and active-strip separators after dynamic clears.
- Preserves the v1.5.8 latest-state-wins RF, asynchronous ACK, 1 ms input task,
  output-before-ACK receiver ordering, and backlight-off full-frame rendering.

## v1.5.7
- Rebuilt the native 170 x 320 screen directly from the approved design master.
- Added the live ACTIVE / RX / RSSI / AGE strip in the previously underused area.
- Added crisp native telemetry rendering to reduce fuzzy text.
- Matched all six relay cards to equal 52 x 64 geometry.
- Preserved first-valid-packet ON response before ACK transmission.
- Preserved guarded multi-packet OFF confirmation to prevent held-button cycling.
- Preserved live packet RSSI, SNR, age, battery, packet count, error count, and uptime.

## v1.5.1
- Restored the approved overall layout after the compact v1.5.0 reinterpretation.

## v1.5.0
- Added the sixth relay output and 3 x 2 card arrangement.


## v1.5.7
- Main screen rebuilt from the user-approved canonical PyrotechnicFX dashboard design.
- Wordmark, subtitle, telemetry row, TX/RX panel, active-status strip, and 6-relay layout aligned to the new canonical mockup.
- Footer text remains removed.


## v1.5.7
- Authenticated OFF commands are applied on the first valid packet; the previous three-packet OFF confirmation delay is removed.
- Receiver output and graphics now follow fast and slow momentary button operation without added release latency.


## v1.5.7
- Replaced corrupted boot image containing the gray wordmark obstruction.
- Boot and first main frame are rendered with the backlight off, eliminating progressive drawing and transition glitches.
- Main-screen dynamic overlays and relay-card coordinates now match the native 170x320 canonical layout.
- TX buttons are scanned in an independent 1ms high-priority task; press and release state is published before UI or Serial work.
- RX applies relay output and transmits ACK before drawing the card, preventing TFT rendering from causing sender ACK timeouts.


## v1.6.3
- Replaced the receiver's tight blocking `radio.receive()` polling loop with DIO1 interrupt-driven `startReceive()` / `readData()` handling.
- The RF task now blocks on a FreeRTOS notification and explicitly yields Core 0, preventing idle-task starvation and repeated watchdog resets.
- Receiver failsafe, immediate output application, ACK ordering, dashboard, boot progress, battery display, and relay graphics are unchanged.
