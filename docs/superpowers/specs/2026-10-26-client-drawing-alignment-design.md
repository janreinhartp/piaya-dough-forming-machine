# Client-drawing alignment

| Field | Value |
| --- | --- |
| Date | 2026-10-26 |
| Product | Piaya Dough Maker prototype |
| Status | Design for a documentation revision. The six control documents are unchanged until this spec is approved and the implementation plan is written. |
| Source drawing | `C:\Users\janre\Downloads\design-1.pdf`, the client mechanical set titled on its sheets as the design and development of a piaya dough making machine. |

## 1. Purpose

The control documents in `docs/01` through `docs/06` are the design of record for behavior, the touchscreen, recipes, timers, and the hardwired emergency stop. The client PDF is the design of record for geometry and mechanical part names. This revision makes those two records sit side by side without one silently replacing the other.

The revision adds one alignment document, copies the client PDF into the repository, and applies small edits where the control documents contradict each other or would hide a mismatch.

## 2. Boundary

In scope:

- `docs/07-Client-Drawing-Alignment.md`
- A repository copy of the client PDF
- Surgical edits to `README.md` and `docs/01` through `docs/06`
- Locked engineering decisions that the client sheets do not mention
- Chosen readings for the contradictions named in section 8

Out of scope:

- Firmware, ESP-IDF files, and a build
- A wiring diagram that names drive terminals
- Valve catalog numbers, a priced bill of materials, and coil-current or fuse ratings
- A claim that the machine meets ISO 13849, IEC 62061, IEC 60204, or any other safety standard
- Renaming the control sequence to the drawing's part names
- Adding a hopper state, a second flattener output, or a second cutter output
- Guessing air pressure, pneumatic bore, pneumatic stroke, valve flow, or which axes fall

A figure is **Verified** only when a manufacturer document already cited in `docs/04-Hardware-Design.md` supports it. A dimension transcribed from the client PDF is **Client drawing**. Client geometry is never labeled **Verified**.

## 3. Files

| Path | Role |
| --- | --- |
| `docs/client/piaya-dough-making-machine.pdf` | Byte copy of the client PDF. The alignment document cites this path and the sheet title, so a later reader does not depend on the Downloads folder. |
| `docs/07-Client-Drawing-Alignment.md` | Part-name map, transcribed geometry, mismatch table, accepted decisions, and the list of facts that stay blank. |
| `README.md` | One table row and a status sentence that point at document 07. |
| `docs/01` through `docs/06` | Related-document rows that point at document 07, the contradiction fixes in section 8, and the decision-status edits in section 7. |

Document 07 uses the same header block as the other documents: document name, product, status, date. Its status is `Alignment record. Control behavior remains in documents 01 through 06.`

## 4. How a callout is transcribed

The PDF has no text layer. Each sheet is read from the page image.

For every titled sheet, document 07 records the sheet number, the sheet-content title, and the title-block project title as they are printed. Untitled CAD views are listed as model views and are not given invented sheet numbers.

For every dimension, diameter, angle, and leader label that is legible, document 07 records the sheet, the view name, the callout text, and the value. A callout that cannot be read is recorded as `unreadable`. An unreadable callout is not replaced with a number from memory or from an earlier image summary.

The dough-barrel sheet and the pneumatic-actuator sheets stay in different rows. A diameter of the dough barrel is not copied into a pneumatic bore, stroke, or valve-flow field in document 04.

## 5. Part-name map

The control documents keep these names. Document 07 maps each one to the client part. The map is filled from the printed labels during transcription. The expected pairs, confirmed only if the printed label matches, are:

| Control name | Client part the sheets are expected to show |
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

If a printed label differs, the printed label wins in document 07 and the control name stays. The mismatch table records the difference.

## 6. Mismatches that must be recorded

Each row below becomes one mismatch in document 07, with the sheet title, what the sheet shows, what the control document says, and the resolution `Control sequence unchanged`. The client column is corrected to the printed sheet during transcription. The row stays. Transcription may add rows. It may not delete M1 through M8.

| Id | What the client sheets show | What the control design keeps |
| --- | --- | --- |
| M1 | The control-panel sheet's LCD and push buttons, as labeled on that sheet | Waveshare ESP32-S3-Touch-LCD-7B, 1024 × 600, plus physical START, STOP, firmware RESET, and a separate hardwired emergency-stop reset |
| M2 | A sigma-blade mixer, a belt motor, and a gate | A mixer-drive run command and a timed pneumatic door with a closed switch, a partial-open command, and a full-open command |
| M3 | A horizontal pneumatic piston and a separate dough barrel | Timed plunger extend, center, and optional retract into the forming barrel |
| M4 | A shutter cutter | Timed cutter extend, optional dwell, and retract |
| M5 | A pneumatic press with pressing plates | One flattener solenoid: down, dwell, up |
| M6 | A conveyor belt | A timed conveyor-drive run. The sheets do not name a variable-frequency drive |
| M7 | A hopper | No hopper actuator and no hopper state |
| M8 | Whatever the titled sheets show about guards and a partial-open stop. Absence is recorded as absence | Fixed guards before any transfer mix with the door open, and a mechanical stop before a dough trial uses the partial-open command |

If the second read finds a guard, a partial-open stop, a drive name, or a second press cylinder, that finding is added as its own row. A second press cylinder does not add a second flattener output in this revision. Two shutter openings support the existing nozzle-count default of 2; document 02 then labels that default **Client drawing** only after the two openings are read on the sheet. If the openings are unreadable, the default stays **Assumption**.

## 7. Decisions locked by this revision

These decisions are written as accepted in `docs/06-Implementation-Plan.md` and, where the sentence still says the item is awaiting approval, in the document that owns the sentence. Document 07 repeats them under `Decisions accepted here` and states that the client sheets do not show them.

| Decision | Accepted reading |
| --- | --- |
| D1 | Documents 01 through 06 remain the control design of record. Document 07 is the alignment record. |
| D2 | The controller target remains touch SKU 31726. Phase 1 still confirms the board in hand. |
| D3 | P15 is the drive-fault input. Both drive fault contacts are in series, closed when both drives are healthy, and the expander pin is low in that state. |
| D4 | PCF8575 outputs are active low. The device is powered at 3.3 V. The 7-bit address is `0x20`. Input sense is the named-condition table in section 8.6. |
| D5 | The GPIO6 output-enable watchdog is mandatory equipment protection for this prototype. Its part number and timeout stay blank. |
| D6 | The cutter valve used by the control design is a 5/2 single-solenoid spring-return valve. With air present, the spring-selected port is retract. P7 is unused and is written inactive. M4 records that the sheet shows a shutter and does not show this valve. |
| D7 | Barrel ready is an operator confirmation. No barrel sensor is added. |
| D8 | The safety relay keeps its own reset. Firmware RESET is a separate control. Neither one performs the other. |
| D9 | The prototype stays English only, with eight recipes, no live process edits during a batch, and no speed command to a drive. |
| D10 | Air preparation is a lockable shutoff, filter, regulator, gauge, and lockable exhaust, with no lubricator. Whether the compressor is oil-free stays blank. Operating pressure stays blank. |
| D11 | Fixed guards are required before any dough trial in which the mixer runs with the door switch clear. |
| D12 | A dough trial that uses the partial-open command requires a mechanical stop. The opening distance stays blank unless a legible callout gives it. |
| D13 | Operation is forbidden on any axis a mechanical review says can fall, until a restraint or a pilot-operated check is fitted. This revision does not assert which axes fall. |
| D14 | Firmware remains a separate approval. This revision creates no firmware. |

## 8. Chosen readings for contradictions

These are the only behavior changes inside the six documents. Each edit replaces the conflicting sentence. It does not add a new operating mode.

### 8.1 Maintenance mixer jog

`docs/01` and `docs/02` allow the mixer to run with the door switch clear only in the two transfer-mix steps, behind fixed guards. `docs/05` also allows a maintenance mixer jog with the door switch clear after an on-screen acknowledgement.

The product rule wins. A maintenance mixer jog requires the door-closed switch on every poll. The on-screen acknowledgement is removed. The transfer-mix states remain the only exception, and D11 still requires guards before that exception is used with a moving agitator.

### 8.2 Calibration and the piece count

`docs/05` section 3 sends every flattener-up completion into `FORM_COUNT`, which adds cycles and pieces. Section 7 says a calibration run reuses the forming states and does not add those pieces to the production batch.

The calibration rule wins. A calibration run is entered from maintenance and carries a sample counter. Completing the flattener-up timer increments the sample counter and returns to the plunge state until the sample count is met, then returns to maintenance with outputs inactive. The run does not enter `FORM_COUNT`, does not change the production piece count, does not write the production checkpoint, and does not set the interrupted-batch flag. `docs/02` section 6 gains the same paragraph so the two documents describe one run.

### 8.3 Piece-count checkpoint

`docs/02` says the firmware may store the completed counts at a successful piece-count update. `docs/05` faults the batch when that write fails.

The checkpoint is required. On every successful `FORM_COUNT` the diagnostics task writes the completed counts and the interrupted-batch flag. A failed write raises `NVS_SAVE_FAIL`, enters `FAULT`, and keeps the count in RAM. The word `may` is removed.

### 8.4 Engineering screen

`docs/02` section 5 lists the screens and omits Engineering. `docs/05` already has that screen behind the engineering PIN.

Engineering is added to the `docs/02` screen list, reached only through the engineering PIN.

### 8.5 Illegal commands and rejected edits

`docs/03` says an invalid command is ignored and, where `docs/02` requires it, raises `SETTINGS_REJECTED`. `docs/02` defines `SETTINGS_REJECTED` as an edit outside its limits.

An illegal command for the current state is dropped and raises no alarm. A full HMI command queue rejects the new command on the screen and raises no alarm. An out-of-range edit raises `SETTINGS_REJECTED` and keeps the previous saved value.

### 8.6 Named input conditions

The I/O map in `docs/04` gives each input a named condition that is true when the pin is low. STOP is low when the button is not pressed, so an open wire is a stop request. A single rule that every function is requested by a low pin would treat that open wire as a released button.

`docs/02` and `docs/05` gain one table:

| Input | Named condition true when the pin is low |
| --- | --- |
| Door closed | Door is closed |
| Safety-relay monitor | Relay is in the run state |
| Air pressure | Pressure is OK |
| START | Button is pressed |
| STOP | Button is released and the wire is intact. Stop is requested when this condition is false |
| Firmware RESET | Button is pressed |
| Drive fault | Both drives report healthy. A drive fault is present when this condition is false |

Door, safety monitor, air, STOP, and drive fault use the table as compiled behavior. They are removed from the editable input polarity mask in `docs/02` section 3.3 and from the engineering screen in `docs/05`. The editable input mask contains only START and firmware RESET, and its factory value matches the table. Output polarity stays the existing mask, factory active low.

### 8.7 Output exclusion

`docs/03` says an automatic request that sets both coils of a pair records a fault. `docs/05` says `field_io` clears both coils and returns an error. No alarm code exists for that error.

An automatic request that sets both ends of a pair forces the inactive image, records `OUTPUT_EXCLUSION`, and enters `FAULT`. A maintenance jog that asks for the opposite coil of a pair is rejected, the first coil stays as the held press requires, and no alarm is raised. `OUTPUT_EXCLUSION` is added to the alarm tables in `docs/02` and `docs/05` with this recovery text: the outputs were forced off. Press firmware RESET. Start a new batch only after the request that set both coils is corrected.

### 8.8 Cutter valve text

`docs/04` currently draws the detented 5/2 as the current design and the spring-return valve as a recommendation awaiting approval. D6 reverses that status. The spring-return valve is the current control design. The detented valve is the rejected alternative. P7's row says unused and written inactive. The failure table's detented-cutter sentences are rewritten so a power loss returns the valve spool toward retract while air remains, and so a loss of air still does not prove the blade moved.

### 8.9 Door close after a finished batch

A door-close timeout from `BATCH_DONE` enters `FAULT` and leaves the stored batch record in place. Firmware RESET then goes to `IDLE`. The finished batch is not resumed and its record is not deleted.

## 9. Facts that stay blank

Document 07 and document 06 keep these blank. None of them receive a planning number in this revision.

- Variable-frequency-drive manufacturer, model, manual, terminal names, sink or source, and fault-contact rating
- Pneumatic bore, stroke, load, and required stroke time for the door, plunger, cutter, and flattener
- Operating air pressure, valve flow, port size, coil current, supply current, and fuse rating
- PCF8575 module schematic, onboard helper-chip identity, backlight register, and GPIO6 header voltage
- Watchdog part number and timeout
- Oil-free compressor status
- Which axes fall when air or electrical power is lost
- Enclosure, washdown, and ambient temperature
- Any client callout marked `unreadable`

Phase 1 remains the place those blanks are filled from the board, the manuals, and the machine. A blank row still blocks purchase and wiring of that item. It does not block writing document 07.

## 10. Edits by file

| File | Edit |
| --- | --- |
| `README.md` | Add document 07 to the document table. Point the status paragraph at the alignment record and at the blanks in section 9. |
| `docs/01-Product-Requirements.md` | Add **Client drawing** to the label legend. Add document 07 to the related-document table. The existing transfer-only door exception stays as written. |
| `docs/02-Technical-Requirements.md` | Apply sections 8.2, 8.3, 8.4, and 8.6. Add `OUTPUT_EXCLUSION` to the alarm list. Add a requirements-trace row that points at document 07. Update the nozzle-count note only under the rule in section 6. |
| `docs/03-System-Architecture.md` | Apply section 8.5. Point the exclusion step at `OUTPUT_EXCLUSION`. Add document 07 to the unresolved-items paragraph as the place client geometry lives. |
| `docs/04-Hardware-Design.md` | Apply sections 8.6 and 8.8. Add **Client drawing** to the label legend. Add document 07 beside the open pneumatic rows, with the rule that barrel geometry is not a pneumatic rating. |
| `docs/05-Software-Design.md` | Apply sections 8.1, 8.2, 8.6, 8.7, and 8.9. Add the host tests in section 11. |
| `docs/06-Implementation-Plan.md` | Split section 5 into decisions accepted in this revision (section 7 of this spec) and facts still blank (section 9 of this spec). Add a host-test line for the calibration bypass and the maintenance door rule. |

No other paragraphs are rewritten. Cross-references that already agree are left in place.

## 11. Tests the documents must name

`docs/05` section 10 gains these host tests, beside the tests already listed:

- A maintenance mixer request with the door switch clear leaves the mixer run bit off.
- A calibration run of the configured sample count leaves the production piece count and the interrupted-batch flag unchanged.
- After the flattener-up timer in a production cycle, the piece count increases only in `FORM_COUNT`.
- A STOP pin that is not low is a stop request.
- An automatic request for both door coils enters `FAULT` with `OUTPUT_EXCLUSION` and writes the inactive image.
- A safety-monitor loss from a forming state enters `ESTOP`.

`docs/06` phase 6 names the same cases in its acceptance line. Phase 12 names that a calibration confirm stores the pulse and does not create a production batch.

## 12. Acceptance of the revision

The revision is complete when all of the following are true:

- `docs/client/piaya-dough-making-machine.pdf` is a byte copy of the client PDF.
- Document 07 contains the transcription rules' output, the part-name map, mismatch rows M1 through M8, the accepted decisions D1 through D14, and the blank list in section 9.
- Every legible callout on a titled sheet appears in the geometry table, and every illegible callout appears as `unreadable`.
- The sentences named in section 8 have been replaced, and the removed readings are gone.
- A search of `docs/` finds no pneumatic bore, coil current, air pressure, valve flow, or drive terminal that this revision invented.
- A search finds no client dimension labeled **Verified**.
- The README links document 07.
- No file under a firmware tree was created.

## 13. What the later implementation plan covers

One implementation plan writes document 07, copies the PDF, and applies section 10. It does not start phase 1 of `docs/06`, and it does not create source code. The plan's review step is a read-through of document 07 against the PDF pages and a read-through of each section 8 edit against the sentence it replaced.
