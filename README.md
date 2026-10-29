# Piaya Dough Maker

Prototype design for a touchscreen-controlled Piaya dough maker. The machine mixes about 15 kg of dough, transfers it into a forming barrel, and then forms, cuts, conveys, and flattens pieces. Cylinder motion is timed. The mixer door is the only actuator with a position switch.

This repository holds the engineering documents and the controller in `firmware/`. The ESP-IDF image builds on this PC. No board boot log has been recorded.

## Safety

The emergency stop is a hardwired safety relay. It does not depend on the ESP32, the PCF8575, the touchscreen, or the firmware. These documents do not claim that the machine meets ISO 13849, IEC 62061, or IEC 60204. A risk assessment is still required before production use.

Timed valve commands do not prove cylinder position or piece weight. The screen design keeps commanded states and measured states separate.

Read [docs/04-Hardware-Design.md](docs/04-Hardware-Design.md) before wiring anything. Several rows in that document are still open, including the VFD manuals, cylinder sizes, and the onboard I/O-helper chip.

## Documents

| Document | What it decides |
| --- | --- |
| [docs/01-Product-Requirements.md](docs/01-Product-Requirements.md) | Process, operator capabilities, timer-only limits, and scope |
| [docs/02-Technical-Requirements.md](docs/02-Technical-Requirements.md) | Settings, permissives, HMI rules, calibration, access, and storage |
| [docs/03-System-Architecture.md](docs/03-System-Architecture.md) | Control layers, software components, tasks, and data flow |
| [docs/04-Hardware-Design.md](docs/04-Hardware-Design.md) | Verified board and expander data, I/O map, circuits, pneumatics, and bill of materials |
| [docs/05-Software-Design.md](docs/05-Software-Design.md) | Drivers, state table, interlocks, screens, alarms, and tests |
| [docs/06-Implementation-Plan.md](docs/06-Implementation-Plan.md) | Phases 1–14, commissioning checklist, and decisions still open |
| [docs/07-Client-Drawing-Alignment.md](docs/07-Client-Drawing-Alignment.md) | Client part names, transcribed geometry, mismatches, and accepted decisions |

## Hardware summary

- Controller: Waveshare ESP32-S3-Touch-LCD-7B, touch SKU 31726, 1024 × 600, ESP32-S3-WROOM-1-N16R8 with 16 MB flash and 8 MB PSRAM.
- Field I/O: one PCF8575 at I2C address `0x20` on the board header, powered at 3.3 V, outputs active low. Address `0x24` is already used on the board.
- Motors: mixer and conveyor variable-frequency drives, run by isolated dry contacts. Speed stays on the drive keypad.
- Pneumatics: 5/3 closed-center valves for the door and plunger, a 5/2 spring-return valve for the cutter with the spring port plumbed to retract, and a 5/2 spring-return valve for the flattener. Valve size is not selected yet. P7 is unused.
- Safety: hardwired emergency-stop relay in series with field power, plus a GPIO6 watchdog that is mandatory equipment protection for this prototype. Its part number and timeout stay blank. It drops field power if the controller stops running.

Confirm the board in hand before ordering interface parts. The non-touch SKU 31727 is the wrong board.

## Development environment

- ESP-IDF 5.5.x. The image in this tree was built with 5.5.4.
- C, CMake, FreeRTOS, and NVS, as provided by ESP-IDF.
- LVGL 8.4, taken from the local Waveshare sample at `docs/demo-firmware/Demo`. That directory is gitignored and must be present for `idf.py build`.
- Host tests use the Visual Studio C compiler through `firmware/test/host/build.ps1`. They do not flash the board.

Arduino and PlatformIO are not the project framework. Wi-Fi, Bluetooth, Modbus, and cloud services are out of scope.

### Waveshare sample

The product links the local sample at `docs/demo-firmware/Demo` for LCD, touch, I2C, the onboard helper through `IO_EXTENSION`, and the LVGL port. That directory stays gitignored and must be present for `idf.py build`. The sample Wi-Fi, SD, SquareLine UI, and `pwm_init` (GPIO6 LED) are not linked. GPIO6 in the product is the watchdog kick only.

From `firmware`, after the ESP-IDF export script:

```powershell
idf.py build
```

The image is `firmware/build/piaya_dough_maker.bin`. Flash it only after the board in hand is confirmed as touch SKU 31726.

## Status

### Done

- Documents 01 through 07, including the client-drawing alignment.
- Host tests for the PCF8575 shadow, field I/O, recipe checks, calibration math, and the state machine in software design section 10.
- ESP-IDF project `piaya_dough_maker`: Waveshare clock settings, a 64 KB NVS partition, the sample LCD, touch, and LVGL port, a GPIO6 kick, the machine and field-I/O tasks, and a dashboard with state, alarm, pieces, and Start, Stop, Reset, Barrel, and Ack.

### Still to do

Phase 1 is still open. These blanks block purchase and wiring of that item. The full list is in [docs/06-Implementation-Plan.md](docs/06-Implementation-Plan.md) section 5.1.

- Confirm the board in hand, the helper-chip marking at address `0x24`, and both VFD manuals.
- Pneumatic bore, stroke, coil current, air pressure, valve flow, fuse rating, oil-free compressor status, and which axes can fall.
- Watchdog part number and timeout. The GPIO6 kick in firmware is only the software side.
- Enclosure, washdown, ambient temperature, and any client callout marked unreadable in document 07.

Bring-up on the board:

- Flash the image and record a USB-UART boot log.
- Show the panel, check touch, and leave it running for 30 minutes.
- Scan I2C and confirm `0x24`, `0x5D`, and `0x20` with field power off.
- Wire one opto input, one opto output, and the GPIO6 watchdog contact. Confirm a missing kick drops field power.

Software still short of the design:

- Save recipes, the commissioned flag, the batch record, and the alarm ring in NVS. The device still boots from the compiled factory values in RAM. Diagnostics only logs.
- Add the remaining screens: Automatic, Recipes, Settings, Manual, I/O, Alarms, Calibration, Batch statistics, Maintenance, and Engineering. The dashboard still needs separate commanded and measured columns.
- Keep output images out of NVS. Keep widget code off the PCF8575.

On the machine, after the blanks above are filled:

- Drive run contacts from the manuals, with no automatic restart after mains returns.
- Stroke the door, plunger, cutter, and flattener one at a time at low pressure.
- Run mix, transfer, forming, counting, and calibration on the machine. Guards are required before a transfer mix with the door switch clear.
- Fill the fault-injection record, then run one supervised batch. Set commissioned only after the trial times replace the factory placeholders.

The phase tests and the commissioning checklist are in [docs/06-Implementation-Plan.md](docs/06-Implementation-Plan.md).
