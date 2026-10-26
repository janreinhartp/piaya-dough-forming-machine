# Technical Requirements

| Field | Value |
| --- | --- |
| Document | 02-Technical-Requirements.md |
| Product | Piaya Dough Maker prototype |
| Status | Planning baseline. Firmware is not started. |
| Date | 2026-10-26 |

This document is the behavioral specification for firmware and the touchscreen. Hardware ratings are in [04-Hardware-Design.md](04-Hardware-Design.md). State-by-state actions are in [05-Software-Design.md](05-Software-Design.md).

Labels mean:

- **Verified** — taken from a cited manufacturer document.
- **Assumption** — planning choice still awaiting confirmation.
- **Recommendation** — unvalidated engineering default. It does not authorize a batch.
- **Open** — not decided.

Factory recipes ship with `commissioned = false`. Start is refused until engineering saves commissioned values from a real trial. The ranges below are **Recommendation** limits so the keypad has bounds. They are not measured process times.

## 1. Controller requirements

| Item | Requirement | Status |
| --- | --- | --- |
| Controller | Waveshare ESP32-S3-Touch-LCD-7B, touch SKU 31726 | **Verified** product identity. Confirm the board in hand. |
| Framework | ESP-IDF, CMake, C, FreeRTOS, NVS, LVGL 8.4 as used by Waveshare example `13_lvgl_v8_demo` | **Verified** that the example exists. Exact ESP-IDF patch version is **Open** until the demo archive is inspected. Expected family 5.5.x. |
| Tooling | VS Code with the Espressif ESP-IDF extension | **Recommendation** |
| Language | C for drivers, sequencing, storage, and the HMI | **Recommendation**. Application C++ is excluded. |
| Display | 1024 × 600 landscape, capacitive touch | **Verified** panel size. Mounting rotation is **Open**. |
| Field I/O | One PCF8575 at 7-bit address `0x20` on the board I2C header | **Recommendation** pending module schematic |
| Persistence | ESP-IDF NVS, versioned blobs | **Verified** that NVS is the ESP-IDF store |
| Networking | Radios off. No Modbus, no speed reference, no encoder | Requirement |

The official demo archive is the firmware starting point. LVGL 9, shipped in a different Waveshare example, is not mixed into this project.

## 2. Permissives

Start and every new output command require:

| Permissive | Source | Fail action |
| --- | --- | --- |
| Safety relay in the run state | P10 low | Enter emergency-stop state. Hardware has already removed energy. |
| Air pressure OK | P11 low | Fault. Remove run commands and write solenoids inactive. |
| Drives healthy | P15 low | Fault. Same output removal. |
| Field I/O communicating | Successful PCF8575 transaction within the poll period | Fault. Retry the inactive image. The GPIO6 watchdog is the hardware backstop if the retry path is dead. |
| Recipe snapshot valid and commissioned | NVS snapshot taken at precheck | Refuse start. Stay idle. |
| Door closed | P9 low | Required for precheck and for the entire mix. Not required after the partial-open command has begun, and only inside transfer. |
| No latched fault | Machine state | Refuse start until reset and the cause is clear. |

Maintenance jogs use the same permissives plus the maintenance mode flag and a current press. They also obey coil mutual exclusion.

The mixer may run with the door switch clear only in the transfer mix steps. Fixed guards are required before that sequence is run with dough or with a moving agitator. See [01-Product-Requirements.md](01-Product-Requirements.md).

## 3. Settings catalog

Internal storage is milliseconds for time and grams for weight. The operator edits time in seconds with 0.1 s resolution, and weight as whole grams. A machine setting can show milliseconds for engineering checks. Storage does not change with that display option.

Every editable row shows the name, the saved value, the value being edited, the unit, the minimum, the maximum, a one-line explanation, Apply, and Cancel. A value outside its limit is rejected in text and is not saved. The previous saved value stays in use. The software does not silently clamp.

Cancel discards the edit. Delete, counter reset, recipe restore, and calibration save each ask for confirmation.

### 3.1 Recipe fields

An operator may edit these. VFD frequency is not a field. Drive speed stays on the drive keypad.

| Field | Unit stored | Operator unit | Initial limit (Recommendation) | Notes |
| --- | --- | --- | --- | --- |
| Name | 24 characters | text | 1–24 characters | Unique among the eight slots |
| Mix time | ms | s, 0.1 | 1.0–1800.0 s | Door must stay closed |
| Transfer mix 1 | ms | s, 0.1 | 0.1–600.0 s | Door interlock exception |
| Transfer mix 2 | ms | s, 0.1 | 0.1–600.0 s | Door interlock exception |
| Door partial open | ms | s, 0.1 | 0.1–60.0 s | Command time only |
| Door full open | ms | s, 0.1 | 0.1–60.0 s | Command time only |
| Door close | ms | s, 0.1 | 0.1–60.0 s | Used by the explicit close command, not by the automatic transfer order |
| Plunger push 1 | ms | s, 0.1 | 0.1–60.0 s | Transfer |
| Plunger retract 1 | ms | s, 0.1 | 0–60.0 s | 0 skips the retract |
| Plunger push 2 | ms | s, 0.1 | 0.1–60.0 s | Transfer |
| Plunger retract 2 | ms | s, 0.1 | 0–60.0 s | 0 skips the retract |
| Forming pulse | ms | s, 0.1 | engineering min–max | See calibration limits |
| Forming retract | ms | s, 0.1 | 0–60.0 s | After the piece is counted, before the next pulse or batch end. It does not run between the pulse and the cutter. 0 skips it. |
| Cutter extend | ms | s, 0.1 | 0.1–30.0 s | No end-of-stroke sensor |
| Cutter dwell | ms | s, 0.1 | 0–30.0 s | 0 skips dwell |
| Cutter retract | ms | s, 0.1 | 0.1–30.0 s | |
| Conveyor run | ms | s, 0.1 | 0.1–60.0 s | One run per forming cycle |
| Flattener down | ms | s, 0.1 | 0.1–30.0 s | Direction proved at low pressure |
| Flattener dwell | ms | s, 0.1 | 0–30.0 s | Held down |
| Flattener up | ms | s, 0.1 | 0.1–30.0 s | Piece count increments after this timer |
| Target piece weight | g | g | 10–200 g | Planning target 50 g is **Assumption** |
| Nozzle count | count | count | 1–2 | Default 2 is **Assumption** |
| Target pieces | count | count | 1–2000 | Operator batch size. The theoretical yield is a separate estimate |

Initial limits are widened or tightened only from the engineering page, and only inside compiled absolute caps: 0–3600 s for mix-class times and 0–120 s for motion-class times. Those caps are not operator settings.

### 3.2 Machine settings

| Field | Who edits | Purpose |
| --- | --- | --- |
| Door-close margin | Engineering | Added to the explicit Close Door time. If the closed switch is still false when the sum elapses, raise a door-close timeout. A held maintenance jog does not use this timer. **Recommendation** initial margin 500 ms. |
| Input debounce | Engineering | Separate stored values for the door switch, air switch, safety monitor, drive fault, and the three buttons. **Recommendation** initial 30 ms, editable 0–200 ms. |
| Forming-pulse minimum and maximum | Engineering | Clamp for recipe edits and for the calibration hint. **Recommendation** initial 100–5000 ms until trials replace them. |
| Calibration sample count | Maintenance | How many pieces are produced and weighed. **Recommendation** initial 4, range 1–20. |
| Rejected-piece adjustment | Maintenance | A confirmed subtraction from the displayed completed-piece count of the current or last batch. Cannot pass below zero. Does not invent strokes. |
| Brightness | Operator | Applied through the official demo's backlight call once phase 1 identifies that call. Hidden if the helper chip on the board in hand has no brightness command. |
| Log level | Maintenance | ESP-IDF log verbosity. Does not suppress the alarm list. |
| Time display | Operator | Seconds or milliseconds on screen. Stored values stay in milliseconds. |
| Maintenance reminder thresholds | Maintenance | Compared with command counters. A reminder is an alarm-style notice, not a forced stop. |

Screen timeout, language selection, and a choice of startup screen are excluded. The startup screen is the dashboard. The language is English.

### 3.3 Engineering settings

Protected by the engineering PIN:

- Output polarity mask. Default is active low on every output, matching the PCF8575 power-on high.
- Input polarity mask. Default is active low, as wired in [04-Hardware-Design.md](04-Hardware-Design.md).
- The timer and debounce limits in section 3.2.
- Commissioned flag.
- Maintenance PIN and engineering PIN changes, each confirmed by entering the current PIN.

GPIO numbers, the coil-exclusion pairs, the emergency-stop response, and the rule that a reboot does not resume a batch are compiled. They do not appear as settings.

### 3.4 When edits take effect

At a successful precheck the machine copies the selected recipe and the machine settings that affect motion into a RAM snapshot. Later edits are stored for the next batch. This prototype has no live process edits.

Output images are never written to NVS.

## 4. State-behavior requirements

The detailed table is in [05-Software-Design.md](05-Software-Design.md). These rules apply to every state:

- The control task arms a timestamp and leaves the state on a later poll. It does not block for a stroke.
- Only one motion command is active in automatic mode.
- Opposing coils are never both requested. The field-I/O layer drops both if a request violates that rule.
- Emergency-stop monitor loss preempts every state, including fault and maintenance.
- Normal stop from a moving state enters Stopping, writes the inactive output image, and then waits in Stopped.
- A piece is counted only in the count state after the flattener-up timer.
- Boot reads configuration, writes outputs inactive, and enters idle unless the safety monitor is unhealthy. An interrupted-batch flag is displayed and cleared. The sequence is not continued.
- Maintenance is entered only from idle, and only with the maintenance PIN. Exit is allowed only when jog requests are released and the inactive image has been written.

## 5. Human-interface requirements

Screens: Dashboard, Automatic, Recipes, Settings, Manual, I/O, Alarms, Calibration, Batch statistics, and Maintenance.

The dashboard shows machine state, selected recipe, commanded motor and valve bits, measured inputs, batch progress, and the active alarm. Commanded bits and measured bits use different labels. Cylinder positions other than door closed are labeled as commanded or as timer elapsed.

Navigation uses large targets on the 1024 × 600 panel, one consistent back path, and reusable rows for settings. A numeric keypad is the only editor for time, weight, and counts.

Manual controls:

- Require maintenance mode.
- Send a command structure to the machine task.
- Do not call the PCF8575 driver from a widget callback.
- Keep coil exclusion and the permissives in section 2.
- Cannot close the safety relay or hold it in.

The manual page states that cutter position and plunger position are not measured, and that a jammed cutter will not be detected.

## 6. Calibration and batch math

Planning basis, **Assumption**: 15 kg of dough, 50 g target piece, 2 nozzles.

Theoretical estimate, shown as an estimate:

- Piece estimate = floor(15000 / target piece weight in grams).
- Cycle estimate = floor(piece estimate / nozzle count).

The operator's target piece count is the batch stop condition. It may differ from the estimate. The estimate uses a fixed 15 kg charge because charge weight is not a setting in this prototype.

Calibration steps:

1. Select a recipe and a forming pulse inside the engineering limits.
2. Produce the configured sample count with the forming cycle.
3. Weigh the samples together on a scale. The scale is outside the controller.
4. Enter the sample count and the total weight.
5. Average = total weight / sample count. Show average, target, and average minus target.
6. Suggested pulse = current pulse adjusted by 10 percent toward the target, then clamped to the engineering limits. If the average is above the target, the hint shortens the pulse. If it is below, the hint lengthens it.
7. Store the pulse only after the operator confirms.

The 10 percent step is a review hint. It is not a model of dough mass. Sample count of 0 or a total weight of 0 is rejected.

Completed cycles, completed pieces, rejected-piece adjustment, target, and remaining pieces are visible during the batch. Remaining = target pieces minus completed pieces, and is not shown below zero.

## 7. Access

| Action | Operator | Maintenance PIN | Engineering PIN |
| --- | --- | --- | --- |
| Start, stop, acknowledge batch | Yes | Yes | Yes |
| Select and edit recipe process fields | Yes | Yes | Yes |
| Manual jog, calibration, counter reset, alarm reset | No | Yes | Yes |
| Polarity, limits, commissioned flag, PIN change | No | No | Yes |
| Disable hardwired emergency stop | No | No | No |

Default PINs are set at first commissioning and stored in NVS. The document set does not publish a default PIN. A lost engineering PIN is recovered by erasing the NVS partition with the ESP-IDF tools and reloading factory data, which also clears commissioned.

## 8. Persistence and validation

Namespaces: `meta`, `recipes`, `machine`, `calib`, `maint`, `alarms`.

Every blob starts with a schema version. On boot:

- Missing namespace loads factory data and raises a visible uncommissioned condition.
- A version mismatch or a failed integrity check loads factory data, raises `NVS_CORRUPT`, clears commissioned, and stays idle.
- A failed save raises `NVS_SAVE_FAIL` and keeps the last good RAM snapshot. The failed edit is not treated as active.

Validation runs before every save and again on the snapshot at precheck. Invalid data refuses the save or the start.

The compiled factory recipe uses the minimum of each recommendation range. Optional retracts and dwells are 0. Those numbers exist so the blob validates. They are not process targets, and start stays refused until engineering replaces them and sets commissioned.

The alarm ring holds 32 records: code, originating state, and short text. Older records are overwritten. Recipe storage is eight slots.

NVS size in the partition table is 64 KB. **Verified:** Espressif's programming guide recommends at least 12 KB for a usable NVS partition and ships a 24 KB default. **Recommendation:** 64 KB, to hold eight recipes and the alarm ring with spare space. This is not a requirement from the dough process.

At each successful piece-count update the firmware may store the completed counts and an interrupted-batch flag. That checkpoint is the only mid-batch NVS write. It is not an output image and it is not a resume token.

## 9. Alarm requirements

Each alarm has a stable code, a plain-language message, the originating state, and a recovery sentence on the alarm screen. The minimum set:

| Code | Meaning |
| --- | --- |
| `ESTOP_ACTIVE` | Safety-relay monitor is not in the run state |
| `AIR_LOW` | Air-pressure input is not OK |
| `DOOR_OPEN_DURING_MIX` | Door switch opened during mixing |
| `DOOR_CLOSE_TIMEOUT` | Close command finished its time plus margin with the switch still clear |
| `I2C_IO_FAILURE` | PCF8575 transaction failed |
| `DRIVE_FAULT` | Drive-fault input opened |
| `CONFIG_INVALID` | Snapshot failed validation at start |
| `NVS_CORRUPT` | Stored configuration could not be used |
| `NVS_SAVE_FAIL` | A save did not complete |
| `SETTINGS_REJECTED` | An edit was outside its limits |
| `UNCOMMISSIONED` | Commissioned flag is false. Standing banner. Start refused. Reset does not set the flag |

`UNCOMMISSIONED` does not force the fault state, so first power-up is not trapped. The other codes latch until the cause is clear and firmware RESET is accepted. `ESTOP_ACTIVE` clears only after the safety monitor is healthy again and firmware RESET is accepted. Firmware RESET does not re-energize the safety relay.

## 10. Logging and diagnostics

Log state transitions, operator commands, recipe saves, batch completion, faults, I2C errors, and unexpected resets. Do not log every poll. The I/O page shows the last commanded word, the last read word, and the last error code. Command counters on the maintenance page are labeled as commands issued.

## 11. Requirements trace

| Product need | Where it is specified |
| --- | --- |
| Configurable process | Sections 3 and 8 |
| Safe start and stop | Sections 2 and 4, and [04-Hardware-Design.md](04-Hardware-Design.md) |
| No false position claims | Section 5 and [01-Product-Requirements.md](01-Product-Requirements.md) |
| Calibration by weight | Section 6 |
| Persistence | Section 8 |
| Implementation order | [06-Implementation-Plan.md](06-Implementation-Plan.md) |
