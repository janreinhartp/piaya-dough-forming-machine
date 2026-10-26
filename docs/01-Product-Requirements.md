# Product Requirements

| Field | Value |
| --- | --- |
| Document | 01-Product-Requirements.md |
| Product | Piaya Dough Maker prototype |
| Status | Planning baseline. Firmware is not started. |
| Date | 2026-10-26 |

This document states what the prototype is for, who operates it, and what is inside the first build. Numeric hardware ratings and firmware structure are in the later documents. Claims that depend on a measurement or a datasheet are marked:

- **Verified** — read from a manufacturer document named in [04-Hardware-Design.md](04-Hardware-Design.md).
- **Assumption** — used so the design can proceed, and still needs confirmation.
- **Recommendation** — engineering choice, not a measured fact.
- **Open** — blocking for purchasing, wiring, or a firmware phase.
- **Client drawing** — transcribed from [07-Client-Drawing-Alignment.md](07-Client-Drawing-Alignment.md). Not a measurement of the built machine.

## 1. Product summary

The prototype takes about 15 kg of Piaya dough through three stages:

1. Mixing.
2. Transfer from the mixer into the forming barrel.
3. Forming, cutting, conveying, and flattening.

An operator runs the machine from a 7-inch touchscreen. Named recipes hold the process timers and batch targets. Those values are changed on the screen and stored on the controller. A normal batch does not require a firmware edit or a rebuild.

Motion of the pneumatic cylinders is timed. The mixer door has one closed switch. No other cylinder position is measured. The screen must keep commanded motion and measured status visually separate. A timer finishing is not a confirmed position and is not a confirmed piece weight.

Two three-phase motors, mixer and conveyor, are started and stopped through the digital inputs of their variable-frequency drives. Speed is set on each drive's own keypad. This prototype does not send a speed command.

## 2. Users and operating modes

| Mode | Who | What they can do |
| --- | --- | --- |
| Automatic | Operator | Select a commissioned recipe, review it, start a batch, watch progress, and stop. |
| Stopped or idle | Operator | Acknowledge a finished or stopped batch, change recipe process values, and inspect alarms and counts. |
| Maintenance | Maintenance, after a local PIN | Jog actuators, calibrate the plunger pulse, reset maintenance counters, and clear alarms after the cause is gone. |
| Engineering | Engineering, after a separate local PIN | Set polarity, timer limits, debounce limits, and the commissioned flag. |

The PIN only hides local pages. It is not a security control. No PIN can disable the hardwired emergency stop, change a compiled output assignment, or bypass a safety interlock.

Physical pushbuttons are part of the prototype:

- START asks the controller to begin. It still has to pass the prechecks.
- STOP requests a normal stop. The contact is normally closed, so an open wire is a stop request.
- RESET acknowledges a firmware alarm or a normal stop. It does not reset the safety relay.
- Emergency stop is a separate hardwired device. Its reset button belongs to the safety relay, not to the touchscreen.

## 3. Process the machine must perform

### 3.1 Idle and precheck

While idle, the screen shows the selected recipe, the batch target, the machine state, and any standing alarm. Starting is refused until all of the following are true:

- The safety-relay monitor reports the relay in the run state.
- The mixer door closed switch is true.
- Air pressure is reported OK.
- Drive-fault contacts report healthy.
- The selected recipe is valid, saved, and marked commissioned.
- No latched fault is active.
- The field I/O link is communicating.

A failed precheck leaves the machine idle or in a latched fault, with a visible reason. It does not start a motor.

### 3.2 Mixing

1. The operator loads ingredients with the door closed.
2. The machine checks the door-closed switch and holds that requirement for the whole mix.
3. The mixer drive is commanded to run.
4. The drive runs for the recipe mix time.
5. The mixer drive command is removed.
6. The machine enters transfer.

If the door switch opens during mixing, the mixer command is removed and the batch faults.

### 3.3 Transfer

The order below is the process of record. A duration of 0 on either plunger retract skips that retract until a mechanical review sets a real time.

1. Open the mixer door for the partial-open time, then de-energize both door coils.
2. Run the mixer for the first transfer-mix time, then stop it.
3. Extend the plunger for the first push time, de-energize both plunger coils, then retract for the first retract time.
4. Open the mixer door for the full-open time, then de-energize both door coils.
5. Run the mixer for the second transfer-mix time, then stop it.
6. Extend the plunger for the second push time, de-energize both plunger coils, then retract for the second retract time if that time is greater than 0.
7. Wait until the operator confirms that the barrel has enough dough.

There is no dough-level sensor. Step 7 cannot be inferred by the controller. **Open:** what partial-open means in millimetres or angle, whether the first push must finish its retract before the full-open command, and whether the second push must retract before forming.

The mixer is allowed to run with the door switch clear only inside these transfer steps, and only behind fixed guards. That exception is not available in the mixing stage.

The automatic sequence does not command the door closed. An explicit Close Door command, available from idle and from the finished-batch screen, uses the close time and then requires the closed switch. A maintenance jog holds the close coil only while the control is pressed and does not by itself declare the door closed. The next batch cannot start until the closed switch is true again.

### 3.4 Forming

Each cycle runs one actuator command at a time:

1. Extend the plunger for the forming pulse, then de-energize both plunger coils.
2. Extend the cutter, hold for the cutter dwell if that time is greater than 0, then retract the cutter.
3. Run the conveyor for its recipe time, then remove the conveyor command.
4. Command the flattener down, hold for the dwell, then command it up.
5. When the up timer completes, add 1 to the cycle count and add the nozzle count to the piece count.
6. If the forming-retract time is greater than 0, retract the plunger and then de-energize both plunger coils. This retract is after the count. It does not run between the forming pulse and the cutter.
7. Repeat until the piece count reaches the recipe target, or until the operator stops the machine.

The retract in step 6 also runs after the cycle that reaches the target, before the finished-batch screen. A stop or fault before the up timer completes does not add a piece for that cycle. Default nozzle count is 2, so one completed cycle is shown as 2 pieces. Overlapping motions are excluded from this prototype.

### 3.5 End of batch, stop, and fault

At the target count the machine removes motor commands, applies the inactive solenoid image, shows completed and remaining quantities, stores the batch record, and waits for the operator to acknowledge. The next batch needs a new start.

Normal STOP during motion does the same output removal immediately. It does not finish the current stroke. Recovery is a firmware reset and then a new start.

A process fault does the same output removal, stores the fault and the state that raised it, and waits until the cause is clear and the operator resets. The machine does not continue a half-finished cycle.

Emergency stop is performed by the hardwired safety relay. The touchscreen only reports it. Recovery requires the relay's own manual reset, then the firmware reset, then a new start.

A reboot never continues a batch. The last completed piece count may be shown as an interrupted batch. Starting again creates a new batch.

## 4. What the operator must be able to do

- Run automatic and maintenance modes from the touchscreen.
- Keep at least eight named recipes, and create, edit, rename, duplicate, select, save, cancel, restore defaults, and delete one with confirmation.
- Review the selected recipe before start.
- Change every process time and batch target in section 3 without editing source code.
- See batch progress, theoretical yield, and the distinction between an estimate and a count.
- Calibrate the forming pulse with a scale and confirm the stored result.
- See active alarms, alarm history, and I/O diagnostics.
- See maintenance counters that count commands, with the screen stating that the strokes were not confirmed by sensors.
- Recover from a missing or corrupt configuration back to factory defaults, with start locked until the machine is commissioned again.

## 5. Limits of timer-only motion

Timed coils are the prototype's position method. They have these product consequences:

- A 5/3 closed-center valve, used on the door and the plunger, blocks its ports when both coils are off. That is a command to stop moving. It is not a lock. Air compresses, valves leak, and dough or gravity can move the piston after the timer ends.
- The partial-open door position is stable only if a mechanical stop exists and the open timer is long enough to reach it. The piston can still creep away from that stop. A dough trial that uses this command requires that stop. The opening distance stays **Open** unless the alignment record quotes a legible callout.
- Plunger time does not fix stroke or dough mass. Pressure, dough consistency, cylinder size, nozzle size, back-pressure, leakage, and friction all change the result. Two equal pulses can produce different piece weights.
- The cutter valve is a 5/2 single-solenoid spring-return. With air present, dropping the coil returns the spool toward retract. Loss of air does not prove the blade moved. There is no cutter sensor, so an incomplete cut is not detected. The client sheet shows a shutter and does not show this valve. See [07-Client-Drawing-Alignment.md](07-Client-Drawing-Alignment.md).
- The flattener spring returns the valve spool. It moves the cylinder only while supply air is present, and only toward the port that was plumbed as the rest position. Loss of air does not lift a tool that can fall. **Open:** whether the flattener or the door can fall.
- Except for the door-closed switch, the screen must not say that a cylinder has arrived. It may say that a command was issued and that its timer has elapsed.
- Piece weight is corrected by weighing samples and confirming a new pulse. The controller must not pretend the relationship is linear.

These limits are accepted for the first prototype. They are not accepted as a substitute for guards, the emergency stop, or a later decision to add sensors.

## 6. Scope

### In this prototype

- Touchscreen HMI on the Waveshare ESP32-S3-Touch-LCD-7B, touch version.
- ESP-IDF firmware, written later, with LVGL, FreeRTOS, and NVS.
- One external PCF8575 for field commands and status, behind 24 V interface circuits.
- Timed pneumatic sequences for the door, plunger, cutter, and flattener.
- Dry-contact run commands for the mixer and conveyor drives.
- Hardwired emergency stop that does not depend on the controller.
- Recipes, calibration, counters, alarms, diagnostics, and maintenance pages.

### Out of this prototype

- Modbus or any other drive network.
- An analog or digital speed reference to a drive.
- An encoder or other conveyor speed feedback.
- Closed-loop cylinder positioning.
- Wi-Fi, Bluetooth, cloud logging, and remote control. The board radios stay off.
- Arduino or PlatformIO as the project framework.
- A claim that the machine meets ISO 13849, IEC 62061, IEC 60204, or any other safety standard. A risk assessment is still required before production use.
- Valve sizing, coil-current selection, and a priced bill of materials. Those wait on measurements listed as **Open** in [04-Hardware-Design.md](04-Hardware-Design.md).

## 7. Prototype success criteria

The documentation set is the current deliverable. The machine prototype is successful only when all of the following are later demonstrated:

- The display and touch work on the real board using the official Waveshare ESP-IDF example as the base.
- With field power applied, a controller reset leaves solenoid and drive-run loads off.
- Automatic mode can run the mix, transfer, and forming order in this document on simulated I/O, then on the real actuators at low pressure.
- An operator can change a process time, save a recipe, reboot, and see the saved value.
- A scale calibration can be entered and is stored only after confirmation.
- Stop, emergency stop, loss of the I/O link, low air, and a mid-batch reboot each end in a safe command state and require a deliberate new start.
- The emergency-stop test still removes motor enable and solenoid supply when the controller is held in reset.

## 8. Related documents

| Document | Contents |
| --- | --- |
| [02-Technical-Requirements.md](02-Technical-Requirements.md) | Settings, permissives, HMI, storage, and access rules |
| [03-System-Architecture.md](03-System-Architecture.md) | Controllers, tasks, data, and interlocks |
| [04-Hardware-Design.md](04-Hardware-Design.md) | Verified parts, wiring, pneumatics, and bill of materials |
| [05-Software-Design.md](05-Software-Design.md) | State table, screens, drivers, and alarms |
| [06-Implementation-Plan.md](06-Implementation-Plan.md) | Phases, tests, commissioning, and open decisions |
| [07-Client-Drawing-Alignment.md](07-Client-Drawing-Alignment.md) | Client part names, geometry, mismatches, and accepted decisions |
