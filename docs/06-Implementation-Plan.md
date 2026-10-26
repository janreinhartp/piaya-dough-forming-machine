# Implementation Plan

| Field | Value |
| --- | --- |
| Document | 06-Implementation-Plan.md |
| Product | Piaya Dough Maker prototype |
| Status | Planning baseline. Phases below are not started. |
| Date | 2026-10-26 |

Do not start a phase until the previous phase's acceptance line is recorded. Firmware files are not part of this documentation pass.

The design of record is:

- [01-Product-Requirements.md](01-Product-Requirements.md)
- [02-Technical-Requirements.md](02-Technical-Requirements.md)
- [03-System-Architecture.md](03-System-Architecture.md)
- [04-Hardware-Design.md](04-Hardware-Design.md)
- [05-Software-Design.md](05-Software-Design.md)
- [07-Client-Drawing-Alignment.md](07-Client-Drawing-Alignment.md)

## 1. How to record a phase

For each phase, write a short note in the project log or the phase checklist with:

- Date and the person who ran it.
- The deliverable that now exists.
- The test that was run and the observed result.
- Pass or fail against the acceptance line.
- Open risks that are allowed to carry forward, and risks that block the next phase.

A failed acceptance stops the sequence. Fix the cause and repeat that phase's test.

## 2. Phases

### Phase 1 — Validate hardware and mechanical requirements

| | |
| --- | --- |
| Deliverable | A confirmation sheet that names the board revision, the helper-chip marking, the demo source file that drives it, both VFD manuals, cylinder bore, stroke, orientation, and load, coil voltage and current, air pressure, which axes can fall, the partial-open stop if any, and a guarding sketch. |
| Depends on | The documents above, the Waveshare demo zip, the schematic PDF, and the physical machine. |
| Test | Compare the sheet with the chip marks and the manuals. Every **Open** item in [04-Hardware-Design.md](04-Hardware-Design.md) section 2 is either filled or still marked blocking. |
| Acceptance | No wiring diagram is marked buildable while a blocking row is empty. The helper chip is identified from the demo and the board, not from a community guess. |
| Risk | A different PCB lot may use CH422G or CH32V003. Treating the wrong one as fact wastes the display bring-up. |

### Phase 2 — ESP-IDF project skeleton

| | |
| --- | --- |
| Deliverable | An ESP-IDF project copied from the official demo, with the toolchain version pinned to the version that demo requires, `sdkconfig.defaults` taken from the Waveshare list in the hardware document, a partition table that includes 64 KB NVS, and a boot log. No machine logic. |
| Depends on | Phase 1's identification of the demo directory and ESP-IDF version. |
| Test | `idf.py build` succeeds. The board prints a boot log over the USB-UART port. |
| Acceptance | Build is repeatable on a clean tree. Arduino and PlatformIO are not the build system. LVGL 8.4 is the UI library if the v8 demo is the base. LVGL 9 is not mixed in. |
| Risk | 120 MHz DDR is experimental in Espressif's guide. Do not "fix" the clocks in this phase. Phase 3 soaks them. |

### Phase 3 — Display and touch

| | |
| --- | --- |
| Deliverable | The official LCD and touch examples running on this board, then a note of the pixel clock and the helper-chip calls that phase 2's tree will keep. |
| Depends on | Phase 2. |
| Test | Color bars or the LVGL demo are visible. Five-point touch, or as many points as the demo draws, lands under the finger. Leave the panel running for 30 minutes. |
| Acceptance | No persistent shift or blanking during the soak. Brightness follows the demo control, or the note says this board revision has no brightness command. Orientation for the enclosure is written down. |
| Risk | Shared flash and PSRAM bandwidth can shift an RGB panel. Fall back only to a mode Espressif lists for the chip IDs read in phase 1. |

### Phase 4 — PCF8575 driver

| | |
| --- | --- |
| Deliverable | `components/pcf8575` and host tests for the cases in [05-Software-Design.md](05-Software-Design.md) section 2.1. Then a bus scan on the real header. |
| Depends on | Phase 2. The scan also depends on a module whose schematic has been checked for 3.3 V and address `0x20`. |
| Test | Host tests pass. On the board, with 24 V field power off, a scan shows `0x24`, `0x5D`, and `0x20`, and does not show a second device at `0x24`. |
| Acceptance | The driver writes `0xFFFF` after probe. A forced timeout is reported and does not clear the caller's image. |
| Risk | A 5 V pull-up on the module can damage GPIO8 and GPIO9. Do not connect an unreviewed module. |

### Phase 5 — Electrical interface bench

| | |
| --- | --- |
| Deliverable | One opto input and one opto output wired as in the hardware document, plus the series watchdog contact if that recommendation is accepted. |
| Depends on | Phase 4 and reviewed module schematics. |
| Test | Power the expander. Confirm the load stays off. Command the output active and inactive. Open the input wire and confirm the fail-safe reading for that channel. Remove the GPIO6 kick, if fitted, and confirm field power drops. |
| Acceptance | Power-up and controller reset leave the load off. A 24 V field wire never lands on a PCF8575 pin. |
| Risk | Active-high wiring would energize loads at power-on because the expander wakes high. |

### Phase 6 — State machine on simulated I/O

| | |
| --- | --- |
| Deliverable | `components/machine` and `components/field_io` against a fake expander. Host tests from software design section 10 pass. |
| Depends on | Phase 4's fake bus. Does not need the panel. |
| Test | Run the host tests. Inspect a trace of transfer and forming order. |
| Acceptance | Tests cover transfer order, forming order, the count point, STOP including a STOP pin that is not low, fault, emergency stop, `OUTPUT_EXCLUSION`, maintenance mixer jog with the door switch clear, calibration that does not change the production count, and reboot. A failing test blocks phase 7. |
| Risk | Skipping the count-point test is how a partial cycle gets counted. |

### Phase 7 — Settings, recipes, NVS

| | |
| --- | --- |
| Deliverable | `components/config_store` with versioned blobs, factory data, commissioned flag, and alarm-ring storage. |
| Depends on | Phase 2 partition table and phase 6 snapshot structure. |
| Test | Save a recipe, reboot, read it back. Corrupt the blob, reboot, and confirm factory data, `NVS_CORRUPT`, and commissioned clear. Reject an out-of-range value without replacing the previous save. |
| Acceptance | Start is impossible while commissioned is false. Output images are not keys in NVS. |
| Risk | A schema change without a version bump will be read as valid garbage. |

### Phase 8 — LVGL screens

| | |
| --- | --- |
| Deliverable | The screens in software design section 5. Commands travel only on the queue. |
| Depends on | Phases 3, 6, and 7. |
| Test | Edit a time, cancel, and confirm the stored value is unchanged. Apply a legal value and an illegal value. Open manual mode and confirm the widget source does not call the expander. |
| Acceptance | Every process field in the technical requirements has a row. Dashboard labels commanded and measured data differently. |
| Risk | Drawing from the machine task will deadlock the LVGL lock. Keep drawing on the LVGL task. |

### Phase 9 — VFD terminal control

| | |
| --- | --- |
| Deliverable | Two dry-contact run circuits wired to the terminals named in the phase 1 manuals. Drive parameters set so power return does not start the motor. |
| Depends on | Phase 1 manuals and phase 5 drivers. |
| Test | Jog each run contact from maintenance. Remove the contact and confirm the drive stops on its programmed ramp or coast. Cycle mains and confirm the motor stays stopped. |
| Acceptance | No ESP32 or PCF8575 pin touches a drive terminal. The test record quotes the manual's terminal names. |
| Risk | Opening a contactor on the drive output can damage the drive if the manual forbids it. Do not add that contactor in this phase. |

### Phase 10 — One actuator at a time

| | |
| --- | --- |
| Deliverable | Each valve plumbed and stroked at low pressure, with coil cross-interlocks fitted. |
| Depends on | Phases 5 and 9's supply wiring, plus cylinder data from phase 1. Valve flow may still be a temporary orifice if sizing is unfinished. Record that. |
| Test | Door open, center, close, and prove the closed switch. Plunger extend, center, retract. Cutter both directions. Flattener down and up. Try to energize both coils of a pair and confirm the hardware interlock prevents it. |
| Acceptance | Flattener rest position with the solenoid off and air present is the intended up position, or the plumbing is corrected before any further test. Timers are recorded as command times, not as proved positions. |
| Risk | A closed-center valve will not hold a partial door opening without a stop. Do not treat a successful timer as a stable mid-stroke. |

### Phase 11 — Mix and transfer

| | |
| --- | --- |
| Deliverable | The mix and transfer states on the machine, with guards in place before the agitator runs with the door open. |
| Depends on | Phases 8 and 10, and the guarding sketch from phase 1 built in hardware. |
| Test | Run mix with the door switch forced open and confirm `DOOR_OPEN_DURING_MIX`. Run the transfer order. Confirm the machine waits at barrel-ready and does not enter forming alone. |
| Acceptance | The order matches the product requirements. The door-closed exception happens only in the two transfer-mix states. |
| Risk | Running the mixer through an unguarded opening is a pinch and entanglement hazard. This phase is forbidden without the guards. |

### Phase 12 — Forming, counting, and calibration

| | |
| --- | --- |
| Deliverable | Forming cycle, piece count, and the scale workflow. |
| Depends on | Phase 11. |
| Test | One cycle. Confirm the count increases only after the up timer. Stop during the up timer and confirm the count does not increase. Weigh a sample, enter it, cancel, and confirm the pulse is unchanged. Confirm, reboot, and confirm the pulse was stored. |
| Acceptance | Two nozzles add two pieces per counted production cycle. The theoretical yield is labeled estimate. The calibration hint is not applied by itself. A calibration confirm stores the pulse and does not create a production batch. |
| Risk | Equal pulses will not make equal mass. Do not tighten a timer and call the weight solved. |

### Phase 13 — Faults, restart, and interlocks

| | |
| --- | --- |
| Deliverable | A filled fault-injection record for the rows below. |
| Depends on | Phase 12 on the bench. A full dough batch is not required for these tests. |
| Test | Unplug I2C during a commanded output. Press emergency stop while the controller is held in reset and confirm energy is gone. Open the air switch. Press STOP mid-stroke. Reboot mid-cycle. Request maintenance during a batch. |
| Acceptance | Each case matches the response in the hardware failure table and the software state table. The machine never continues the old cycle after reboot. Emergency stop still works with the controller held in reset. |
| Risk | A latched PCF8575 low can hold a coil until the watchdog or the safety relay removes 24 V. The test must look at the load, not only at the screen. |

### Phase 14 — Commission the prototype

| | |
| --- | --- |
| Deliverable | One recorded batch attempt at the real charge, with the times that replace factory placeholders, and commissioned set only after those times are saved. |
| Depends on | Phase 13 passed. |
| Test | Run the product sequence with an operator at barrel-ready. Record actual command times, piece weights, faults, and anything the timers did not hold. |
| Acceptance | The batch either completes to the operator target or stops in a defined fault or stop state. Factory placeholders are no longer the values in the selected recipe. A new start is required after any stop. |
| Risk | This is still a timer-based prototype. Completion does not validate the machine as a production standard design. |

## 3. Unit and integration tests

| Layer | Where | What |
| --- | --- | --- |
| Unit | Host, `test/` | PCF8575 shadow and byte order, field-I/O exclusion and polarity, state order, count point, calibration math, recipe validation |
| Component | Board, field power off | I2C address scan and safe write |
| Integration | Bench, one channel | Input fail-safe and output off at power-up |
| Integration | Drives | Dry contact and no auto-restart |
| Integration | Pneumatics, low pressure | One actuator, then mix and transfer, then forming |
| System | Phase 13 and 14 | Fault injection and a supervised batch |

Host tests do not flash the board. A test that needs 24 V or air is an integration test and is not run from the host runner.

## 4. Commissioning checklist

Use this list in phase 14, and earlier where a line says so.

- Board in hand is SKU 31726, 1024 × 600, touch responds.
- Helper chip and backlight behavior match the phase 1 sheet.
- ESP-IDF version matches the demo pin.
- PCF8575 is at `0x20`, at 3.3 V, and `0x24` is still only the onboard helper.
- Boot with field power on leaves every solenoid and both run relays off.
- GPIO6 watchdog, if accepted, drops field power when the kick stops.
- Emergency stop drops solenoid power and the drive enable or STO path while the controller is held in reset.
- Releasing the emergency-stop button does not restore energy until the safety relay's own reset is pressed.
- Firmware RESET does not move the safety relay.
- STOP is normally closed. An open wire stops the sequence.
- Drive mains cycle does not start a motor.
- Door-closed switch is true only when the door is closed, and mixing faults when it opens.
- Opposing coils cannot both be powered when one software bit is forced, because the hardware interlock is fitted.
- Flattener spring position has been proved at low pressure.
- Guards are fitted before any transfer mix with the agitator moving.
- Recipe edit, reboot, and calibration confirm survive power cycle.
- Corrupt-NVS boot refuses start until commissioned again.
- Mid-batch reboot returns to idle and shows the interrupted count only as information.
- Screen never labels a timer completion as a confirmed cylinder position, except the door-closed input.
- VFD terminal names written on the drawing match the manuals filed in phase 1.

## 5. Decisions accepted in the alignment revision

These readings are accepted. The client sheets do not show them. Detail is in [07-Client-Drawing-Alignment.md](07-Client-Drawing-Alignment.md).

1. Documents 01 through 06 remain the control design of record. Document 07 is the alignment record.
2. The controller target remains touch SKU 31726. Phase 1 still confirms the board in hand.
3. P15 is the series drive-fault input, low when both drives are healthy.
4. PCF8575 outputs are active low, at 3.3 V, address `0x20`. Fail-safe input sense is compiled. The editable input mask is START and firmware RESET only.
5. The GPIO6 watchdog is mandatory equipment protection. Part number and timeout stay blank.
6. The cutter valve is 5/2 spring-return to retract. P7 is written inactive.
7. Barrel ready is an operator confirmation.
8. The safety-relay reset and firmware RESET stay separate.
9. English only, eight recipes, no live process edits, and no drive speed command.
10. Air preparation has a lockable shutoff, filter, regulator, gauge, and lockable exhaust, and no lubricator.
11. Fixed guards are required before a transfer mix with the door switch clear.
12. A partial-open dough trial requires a mechanical stop. The distance stays blank unless document 07 quotes a callout.
13. An axis that a mechanical review says can fall needs a restraint before operation. This plan does not assert which axes fall.
14. Firmware remains a separate approval. The alignment revision creates no firmware.

## 5.1 Facts still blank

Phase 1 fills these from the board, the manuals, and the machine. A blank still blocks purchase and wiring of that item.

- Variable-frequency-drive manufacturer, model, manual, terminal names, sink or source, and fault-contact rating
- Pneumatic bore, stroke, load, and required stroke time for the door, plunger, cutter, and flattener
- Operating air pressure, valve flow, port size, coil current, supply current, and fuse rating
- PCF8575 module schematic, onboard helper-chip identity, backlight register, and GPIO6 header voltage
- Watchdog part number and timeout
- Oil-free compressor status
- Which axes fall when air or electrical power is lost
- Enclosure, washdown, and ambient temperature
- Any client callout marked `unreadable` in document 07

## 6. Risks that span phases

- Timer control will be mistaken for measured position unless every screen and test record uses the commanded-versus-measured wording.
- The shared I2C bus can take the touch and the backlight down with the field expander.
- The expander latches lows across a controller reset. The watchdog and the safety relay are the hardware answers. The screen is not.
- Closed-center valves drift. A partial door opening without a mechanical stop will not stay put.
- Drive wiring copied from a generic diagram can damage a drive or fail to stop it. Phase 1 manuals are mandatory.
- Nothing in this plan is a certification to ISO 13849, IEC 62061, or IEC 60204. A risk assessment is still required before production use.
