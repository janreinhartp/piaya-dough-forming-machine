# Piaya Dough Maker

Prototype design for a touchscreen-controlled Piaya dough maker. The machine mixes about 15 kg of dough, transfers it into a forming barrel, and then forms, cuts, conveys, and flattens pieces. Cylinder motion is timed. The mixer door is the only actuator with a position switch.

This repository currently holds the engineering documents. Firmware has not been started. There is no ESP-IDF project yet.

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

## Hardware summary

- Controller: Waveshare ESP32-S3-Touch-LCD-7B, touch SKU 31726, 1024 × 600, ESP32-S3-WROOM-1-N16R8 with 16 MB flash and 8 MB PSRAM.
- Field I/O: one PCF8575 at I2C address `0x20` on the board header, powered at 3.3 V, outputs active low. Address `0x24` is already used on the board.
- Motors: mixer and conveyor variable-frequency drives, run by isolated dry contacts. Speed stays on the drive keypad.
- Pneumatics: 5/3 closed-center valves for the door and plunger, a detented 5/2 double-solenoid for the cutter unless the spring-return alternative is approved, and a 5/2 spring-return valve for the flattener. Valve size is not selected yet.
- Safety: hardwired emergency-stop relay in series with field power, plus a recommended GPIO6 watchdog that drops field power if the controller stops running.

Confirm the board in hand before ordering interface parts. The non-touch SKU 31727 is the wrong board.

## Development environment

When firmware work is approved, the toolchain is:

- ESP-IDF, pinned to the version required by the official Waveshare demo. The expected family is 5.5.x.
- CMake, C, FreeRTOS, and NVS, as provided by ESP-IDF.
- LVGL 8.4, matching Waveshare example `13_lvgl_v8_demo`.
- VS Code with the Espressif ESP-IDF extension.

Arduino and PlatformIO are not the project framework. Wi-Fi, Bluetooth, Modbus, and cloud services are out of scope.

The first implementation phase is a hardware confirmation sheet, not a firmware tree. See [docs/06-Implementation-Plan.md](docs/06-Implementation-Plan.md).

## Status

Planning documents are in `docs/`. Open decisions include the drive manuals, cylinder data, the cutter valve type, the drive-fault input on P15, and the GPIO6 watchdog. Those decisions are listed at the end of the implementation plan.
