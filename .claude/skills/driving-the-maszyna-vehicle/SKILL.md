---
name: driving-the-maszyna-vehicle
description: How a driver starts a MaSzyna locomotive or multiple unit from cold and moves it off, step by step, with the original engine's conditions behind each step (line contactors only on the first notch, security system after the battery, ground relay needing a direction, main switch held and released, brake pipe charging). Use before writing or judging a start-up test or probe (MaszynaStartupTest), before reading "the vehicle does not start / does not move / the controller does nothing" as a bug, and when scripting a vehicle by keys, cab controls or commands.
---

# Driving a MaSzyna vehicle

Every step below is a condition of the original engine, not a habit of this wrapper. A test or a
probe that skips one sees a vehicle "that does not start" which is only driven wrong - check this
list before calling it a regression. The keys are actions of `demo/project.godot`; the cab logic
takes them (`LegacyCabinLogic.input()`), with or without the 3D cab. `demo/tests/maszyna_startup_test.gd`
drives this sequence.

## From cold to moving off

1. **Battery** - `battery_toggle`. Done: `RailVehiclePowerSupply.get_power24_available()` (the
   flag `battery_enabled` proves nothing - a battery of 0 V switches on too). A car without its own
   battery takes the unit's low voltage.
2. **Cab activation** - only where FIZ `AutomaticCabActivation=No` (36WEa): `cab_activation_toggle`.
   `PlayerServer.player_take_over_vehicle()` sends `cab_activation_auto`, which does nothing then
   (Mover.cpp:2672). Without an active cab `IncMainCtrl()` refuses every step (Mover.cpp:2226).
   Done: `RailVehicleMasterController.get_cabin() != 0`.
3. **Pantographs** (electric) - if the pantograph tank is below
   `get_collector_min_pantograph_tank_pressure()`, hold `pantograph_compressor_activate` until it
   is not; then `pantograph_front_toggle` / `pantograph_rear_toggle`. A cab with a pantograph
   selector (`pantselect_sw`: E186, 36WE) ignores those keys (Train.cpp:3156) - it raises them by
   `pantograph_select_next/previous` and the valves. Done: `get_collector_voltage() > 0` on
   `RailVehicleServer.vehicle_find_pantograph_carrier()`.
4. **Direction** - `direction_increase`, **before** the relay reset: the ground relay resets only
   with a direction set (Mover.cpp:6038-6051).
5. **Relay reset** - `fuse_reset`, `converter_fuse_reset` while
   `RailVehicleEngine.get_main_switch_closable()` is false.
6. **Main switch** - hold `main_switch_toggle` (M, the original's `linebreakertoggle`) for at least the line breaker's InitialCtrlDelay
   (`RailVehicleElectricEngine.get_line_breaker_initial_delay()`), then release: a series motor
   vehicle closes it on the release (`LegacyCabinMainSwitch`, Train.cpp:3809). Diesel: the same
   key starts the engine, after `oil_pump_toggle` and `fuel_pump_toggle`.
7. **Converter, compressor** - `converter_toggle`, `compressor_toggle`; wait for the main
   reservoir (`RailVehicleBrake.get_compressor_pressure()` above 4.5 bar, the AI's ready check).
8. **Security system - acknowledged the moment it blinks, all the time.** The battery arms it
   (Mover.cpp:131) and the vigilance device asks again after its AwareDelay. Left alone it brakes
   in emergency and **empties the brake pipe**: `security_acknowledge` (and
   `security_cabsignal_acknowledge` while the cab signal blinks), as a reflex, not as one step.
   Done: `RailVehicleSecuritySystem.get_braking()` false.
9. **Hand and spring brakes** - some vehicles have a manual brake
   (`RailVehicleBrake.get_manual_position() > 0`: `manual_brake_decrease`), some a spring brake
   (`RailVehicleSpringBrake.get_active()`: `spring_brake_toggle`). A scenery vehicle taken over
   after it stood may be held by its independent brake (Driver.cpp:8166-8180):
   `local_brake_decrease`.
10. **Train brake** - `brake_level_drive`. A lone locomotive fills its brake pipe in **a few
    seconds**; a long wait means something vents or locks it. **An FV4a locks the pipe below
    2.75 bar** until the distributor releases (`lock_old`, Mover.cpp:4033-4041): the driver
    **holds** the releaser (`brake_release`, odluźniacz) while the pipe fills - a tap is not
    enough. Not every locomotive has a hand releaser: newer ones unlock the pipe otherwise
    (`lock_new`, `UnlockPipe`/`HandleUnlock`). Done: `get_pipe_pressure()` near 5 bar, brake
    cylinders empty (`get_air_pressure()`).
11. **Master controller - one notch, then wait.** On a vehicle with neither a camshaft nor
    `TrainType=ezt` the line contactors close **only while the controller stands at its first power
    position for InitialCtrlDelay** (Mover.cpp:6394-6401). Run through the notches at once and they
    never close: position 10, actual position 0, no current, no movement. Step one notch and wait
    until `get_main_actual_position()` reaches `get_main_position()` before the next. An EMU or a
    camshaft vehicle takes any power position.

## The game's own procedures - read these first

The MaSzyna manual describes how each kind of vehicle is started (https://eu07.pl/docs/readme.html
section 7.1 -> https://eu07.pl/docs/inne/readme_pliki/sterowanie.html). What a test or a probe does
follows it; the list above is its general case. In short:

* **Electric locomotives** (EU07, EP07, EP09, ET22): direction (D), battery (J), vigilance
  (Space), pantographs (O/P), relay reset (N), main switch (M) once the line voltage is there,
  converter (X), compressor (C) to 7 bar, charge the brake pipe (Num 4), running position
  (Num 6) to 5 bar, current range (F) where there is one, power (Num +).
  * **181, 182, EP05, EU05, ET40**: the pantographs by the selector (Shift+P / Shift+O,
    `pantograph_select_next/previous`), not by O/P.
  * **EP08**: the pantograph chosen in the engine room (O/P), raised from the cab
    (Ctrl+Shift+O, `pantograph_toggle_selected`). **ET41**: chosen in both engine rooms, raised
    from the cab (Ctrl+Shift+O).
* **EN57, ED72**: battery, the cab activated by its switch, a pantograph (O or P), main switch;
  the converter and the compressor start on their own (to 7 bar); the brake pipe charged by the
  button by the gauges; the spring brake off (right panel); direction; forward I or II.
* **Impuls (31WE, 36WEa, 45WE, ED78)**: battery, cab activation, pantographs (P, O - some by the
  selector, Shift+P/O, then the switch); main switch; converter and compressor on their own (to
  5 bar); the brake lever to DR (Num .); the brake pipe charged by holding the button below the
  SHP lamps until the safety brake lamp goes out; the spring brake released (green button).
* **Elf (EN62, EN76, EN96)**: as Impuls, the **front** pantograph (P); the pipe charged by holding
  the 3.5 bar button until 5 bar; the spring brake release held until its lamp goes out.
* **Diesel-electric** (SM42, SP42, SU45, SU46, ST44, BR285): direction, battery, vigilance, fuel
  pump (F), oil pump (Shift+F); SU45/SU46 also the water pump (W) and the heater (Shift+W) until
  45 C; the starter (M) **held until the engine fires**; the generator excites on its own; charge
  the pipe (Num 4); engine speed up for the air; running position (Num 6); power (Num +) - some
  need two presses, the first only closes the contactors.
* **Diesel-mechanical**: SN61 - direction, master controller to 1, starter (M), back to idle,
  charge the pipe, release, half-clutch to 7-8 km/h, then the gears (Num +). SM03/04 - battery,
  oil pump, starter (M), engine speed up; at 5 bar charge the pipe once and release; direction;
  **first gear (Num /)**; throttle (Num +); next gear at full revs.
* **Draisines**: DL-2 - direction, starter held >10 s, throttle, gear (Num /). WMB10 - battery,
  starter, throttle for the air to 5 bar, running position once, spring brake released,
  direction, first gear, 2500 rpm. EL16 - battery, direction, hand brake off (Ctrl+Num 7), power.

## Which car to read

| What | Car |
|---|---|
| low voltage, cab, direction, speed, security system, brake pipe | the occupied car |
| main switch, converter, compressor, master controller positions | `RailVehicleServer.vehicle_find_powered(occupied)` |
| pantographs, line voltage | `RailVehicleServer.vehicle_find_pantograph_carrier(occupied)` |

On an EMU the control car the player sits in has no engine; commands go to the motor car along the
couplers (`SendCtrlToNext`, Mover.cpp:9085).

## Time

Every wait is simulated time (`SimulationServer.simulation_get_time()`), never frames - a headless
frame is microseconds. Filling the reservoir and the pipe from cold takes minutes of simulated time;
a test runs the clock faster (`SimulationServer.simulation_speed`) and restores it after.

## Learned the hard way - add every new lesson here

Whatever a session learns about driving a vehicle - a step a test or a probe missed, a condition
of the original that made a vehicle "not start" - is written here, with the date, the vehicle and
the original's line, in the same work. The operator should never have to say it twice.

* 2026-10-04, EP07 (start-up test by keys): the controller run through to position 10 at one notch
  a second left the actual position at 0, no current, no movement - the line contactors wait for
  the first notch held for InitialCtrlDelay (Mover.cpp:6394-6401). The operator had said so before.
* 2026-10-04, EP07: a cold vehicle's security system brakes in emergency from its first frame,
  before the battery and without blinking - the pipe drops below 2.75 bar and the FV4a locks it.
  With the handle in running and 8 bar in the main reservoir the pipe stayed at 1.6-2.6 bar for
  600 s; holding the releaser (operator: "odluźniacz trzeba trzymać") filled it in seconds.
* 2026-10-04, test runs at x100: the security reflex has to run every frame - between two steps
  a few frames are seconds of simulated time, enough for the cab signal to brake.
* 2026-10-04 (operator): a lone locomotive fills its brake pipe in a few seconds - a test that
  waits minutes for it is waiting for the wrong thing.
* 2026-10-04 (operator): some vehicles have a manual brake, others a spring brake - both are
  released before moving off. Not every locomotive has a hand releaser - newer ones do not.
* 2026-10-04, every cab: the keys of controls the MMD models went only to the 3D cab's widgets, so
  without the cab model (a fixture, a model that failed to load) they did nothing - the keys are
  the cab logic's now (`LegacyCabinLogic.input()`).
* 2026-10-04, ET41: P/O did nothing from the cab - its pantograph cocks (pantfront_sw,
  pantrear_sw) and its pantograph compressor are in cab0definition, the machine room, and from a
  cab a pantograph is raised only by a switch the cab has (Train.cpp:3228). The driver goes to the
  machine room (`cabin_next`, cab 0), sets the cocks, goes back (`cabin_previous`) and raises them
  together (Ctrl+Shift+O). An impulse switch works its valve only while it is held.
* 2026-10-04, E6ACT, EN57AL (induction motors): the master controller has no actual position to
  wait for - only a series motor's line contactors are waited for, on the first power position.
* 2026-10-04, E6ACT, Impuls, Elf: a newer vehicle locks the brake pipe (lock_new, Mover.cpp:4031)
  and unlocks it by a universal brake button with ub_UnlockPipe (FIZ UBBn, hamulce.h:151; held by
  the hand, no key) or the handle below HandleUnlock - the releaser alone does not.
* 2026-10-04, diesels: a pump in an automatic start mode (FIZ OilPump/FuelPump start) starts with
  the engine; only a manual one is switched. An SN61 starts with its master controller at 1. A
  gearbox (SM03, WMB10, DL-2) takes its first gear (Num /) before the throttle.
* **Not every electric is fed from the wire.** `Power: EnginePower=` says it: an accumulator
  locomotive (EL16) has no pantographs to raise - it runs on its battery. `ConverterStart=Disabled`
  means no converter and no 110 V: skip it, do not wait for it.
* **The pantographs may be on a car without an engine** (31WE/ED78 B and C, 36WEa B): find the
  carrier (`RailVehicleServer.vehicle_find_pantograph_carrier()`) and read its
  `RailVehicleEnginePowerSource`, not the engine of the powered car.
* **An EMU's compressors may be in the control cars**, not in the motor car (EN57KM:
  `CompressorSpeed=0.0` in `s`, the control cars' compressors start on their own once 110 V is
  there). A FIZ without `CompressorPower=` means a converter-fed compressor (the original's 1).
* **A FIZ in MPa cannot be driven** (EP03: `HiPP=0.5`): the brake pipe never passes 3.6 bar and
  the pressure switch keeps the line contactors open - in the original too. That is a data defect.
* **A diesel needs its converter too.** Once the engine runs, the original's AI switches the
  converter on (if there is one) and only then the compressor (Driver.cpp:2799-2806): a compressor
  without `CompressorPower=` runs off 110 V, which only a working converter gives (TEM2). A
  compressor driven by the engine (`CompressorPower=Main/Engine` in a diesel) is slow at idle -
  give the main reservoir minutes, not seconds.
