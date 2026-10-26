# Client Drawing Alignment

| Field | Value |
| --- | --- |
| Document | 07-Client-Drawing-Alignment.md |
| Product | Piaya Dough Maker prototype |
| Status | Alignment record. Control behavior remains in documents 01 through 06. |
| Date | 2026-10-26 |
| Source | [docs/client/piaya-dough-making-machine.pdf](client/piaya-dough-making-machine.pdf) |

This document records the client mechanical drawing. It does not replace the control sequence. A dimension here is **Client drawing**: transcribed from the PDF, not measured on a built machine, and not a manufacturer datasheet. **Verified** is not used in this document.

## 1. Part-name map

The control documents keep the left-hand names. The right-hand names are the labels this record expects on the sheets. Task 2 of the alignment plan corrects a right-hand cell when the printed label differs, and adds a mismatch row for that difference.

| Control name | Client part |
| --- | --- |
| Mixer motor | Belt motor on the sigma-blade mixer |
| Mixer door | Gate |
| Forming barrel | Cylinder with an entry, an exit, and a shaft entrance |
| Plunger | Pneumatic piston |
| Cutter | Shutter cutter |
| Flattener | Pneumatic press |
| Conveyor | Teflon-coated fabric conveyor belt |
| Operator interface hardware | Control-panel casing, LCD, and push buttons |
| Frame | Base |
| Ingredient inlet | Hopper |

## 2. Geometry

Columns are sheet number or `model view`, sheet-content title, view name, callout text, and value. A value that cannot be read is the word `unreadable`. Dough-barrel diameters stay in this table. They are not pneumatic bores.

| Sheet | Title | View | Callout | Value |
| --- | --- | --- | --- | --- |
| | | | | |

## 3. Mismatches

Resolution for every row is `Control sequence unchanged`. The client column is corrected to the printed sheet when transcription disagrees with it. Rows M1 through M8 stay.

| Id | Sheet | What the client sheets show | What the control design keeps |
| --- | --- | --- | --- |
| M1 | Control panel | The control-panel sheet's LCD and push buttons, as labeled on that sheet | Waveshare ESP32-S3-Touch-LCD-7B, 1024 × 600, plus physical START, STOP, firmware RESET, and a separate hardwired emergency-stop reset |
| M2 | Mixing components | A sigma-blade mixer, a belt motor, and a gate | A mixer-drive run command and a timed pneumatic door with a closed switch, a partial-open command, and a full-open command |
| M3 | Pneumatic piston and cylinder | A horizontal pneumatic piston and a separate dough barrel | Timed plunger extend, center, and optional retract into the forming barrel |
| M4 | Shutter cutter | A shutter cutter | Timed cutter extend, optional dwell, and retract. The control valve is spring-return to retract. The sheet does not show that valve |
| M5 | Pneumatic press | A pneumatic press with pressing plates | One flattener solenoid: down, dwell, up |
| M6 | Expected output | A conveyor belt | A timed conveyor-drive run. The sheets do not name a variable-frequency drive |
| M7 | Hopper | A hopper | No hopper actuator and no hopper state |
| M8 | All titled sheets | Whatever the titled sheets show about guards and a partial-open stop. Absence is recorded as absence | Fixed guards before any transfer mix with the door open, and a mechanical stop before a dough trial uses the partial-open command |

## 4. Decisions accepted here

The client sheets do not show these items. They are accepted for the control design.

| Id | Accepted reading |
| --- | --- |
| D1 | Documents 01 through 06 remain the control design of record. This document is the alignment record. |
| D2 | The controller target remains touch SKU 31726. Phase 1 still confirms the board in hand. |
| D3 | P15 is the drive-fault input. Both drive fault contacts are in series, closed when both drives are healthy, and the expander pin is low in that state. |
| D4 | PCF8575 outputs are active low. The device is powered at 3.3 V. The 7-bit address is `0x20`. Input sense is the named-condition table in the technical and software documents. |
| D5 | The GPIO6 output-enable watchdog is mandatory equipment protection for this prototype. Its part number and timeout stay blank. |
| D6 | The cutter valve used by the control design is a 5/2 single-solenoid spring-return valve. With air present, the spring-selected port is retract. P7 is unused and is written inactive. M4 records that the sheet shows a shutter and does not show this valve. |
| D7 | Barrel ready is an operator confirmation. No barrel sensor is added. |
| D8 | The safety relay keeps its own reset. Firmware RESET is a separate control. Neither one performs the other. |
| D9 | The prototype stays English only, with eight recipes, no live process edits during a batch, and no speed command to a drive. |
| D10 | Air preparation is a lockable shutoff, filter, regulator, gauge, and lockable exhaust, with no lubricator. Whether the compressor is oil-free stays blank. Operating pressure stays blank. |
| D11 | Fixed guards are required before any dough trial in which the mixer runs with the door switch clear. |
| D12 | A dough trial that uses the partial-open command requires a mechanical stop. The opening distance stays blank unless a legible callout gives it. |
| D13 | Operation is forbidden on any axis a mechanical review says can fall, until a restraint or a pilot-operated check is fitted. This record does not assert which axes fall. |
| D14 | Firmware remains a separate approval. This revision creates no firmware. |

## 5. Facts that stay blank

No planning number is written for any of these.

- Variable-frequency-drive manufacturer, model, manual, terminal names, sink or source, and fault-contact rating
- Pneumatic bore, stroke, load, and required stroke time for the door, plunger, cutter, and flattener
- Operating air pressure, valve flow, port size, coil current, supply current, and fuse rating
- PCF8575 module schematic, onboard helper-chip identity, backlight register, and GPIO6 header voltage
- Watchdog part number and timeout
- Oil-free compressor status
- Which axes fall when air or electrical power is lost
- Enclosure, washdown, and ambient temperature
- Any client callout marked `unreadable`

## 6. Shutter openings

This sentence is replaced by Task 2 after the shutter sheet is read: `Shutter opening count: not yet read.`

## 7. Related documents

| Document | Contents |
| --- | --- |
| [01-Product-Requirements.md](01-Product-Requirements.md) | Process, operator capabilities, timer-only limits, and scope |
| [02-Technical-Requirements.md](02-Technical-Requirements.md) | Settings, permissives, HMI, storage, and access rules |
| [03-System-Architecture.md](03-System-Architecture.md) | Controllers, tasks, data, and interlocks |
| [04-Hardware-Design.md](04-Hardware-Design.md) | Verified parts, wiring, pneumatics, and bill of materials |
| [05-Software-Design.md](05-Software-Design.md) | State table, screens, drivers, and alarms |
| [06-Implementation-Plan.md](06-Implementation-Plan.md) | Phases, tests, commissioning, and remaining blanks |
