---
name: testing
description: Write, change, run and judge tests in libmaszyna (GUT, demo/tests) and headless probes. Use before adding or editing a test, before "fixing" a test that went red, before running tests ahead of a commit, when a change touches physics (Mover wrapper, vehicle components, FIZ factory, cab logic), and when a headless probe has to prove a hypothesis. Answers "what may a test assert", "is this red test a regression", "how do I run just my test" and "why does my probe read frozen values".
---

# Testing

`AGENTS.md` ("Checks", "Before every commit") and `CODE_STYLE.md` ("Tests") bind as written; this
skill gathers them with what they cost us when they were broken.

## A red test is a suspected regression - ABSOLUTE

A test that turns red after a change is **distrusted, not fixed**. Before touching it:

1. Read what it guarded - its assertions and its setup - find the change that turned it red, and
   report it to the operator: the commit, the line, what it does to the real data. "On purpose"
   is the **operator's** word, never yours: a commit message, a comment citing the original or
   your own reading of the intent does not make a behaviour change wanted. Until the operator
   confirms that exact behaviour, the change is the regression and the test stays as it is - the
   regression is looked for first, the test is changed last.
2. Never make it pass by removing its setup or its assertion, by loosening a tolerance, or by
   giving it a value the code now happens to produce.
3. When the change moved a responsibility (a field to another component, a command to another
   owner), the test follows it to the new owner with the **same** assertion.

2026-10-04, b5e744f1: the battery moved from the controller to `RailVehiclePowerSupply`; five tests
lost `battery_voltage = 110` with nothing in its place and stayed green, because none of them
asserted the low voltage. The tester could not start an EU07, ED78 or 36WE. Breaking this rule
breaks the project's rules.

2026-10-12, 4af6453d (#296): the importer started reading every `.scn` node name in lower case
("as the original reads it") - a change of semantics written into a plan about switches as one of
its decisions, never asked. The EN57 start-up test went red on it; it was called a "test bug", its
names were lowercased and the edit folded into #296 as a fixup, with nobody asking the operator.
The operator's decision came only after the game started players outside their vehicles: names as
the original, lowercased at read time - and the start-up test had been the first to show that the
port did it only halfway (the starter and the trainset's track still read as written).

## The test steps the simulation - no timeouts - PROHIBITED, ABSOLUTE

A test's result never depends on how fast the machine is. On 2026-10-07 the EN57 start-up test
went red on CI and green locally: the simulation then moved with the frames (SimulationRuntime,
real frame time x the speed, up to 25 simulated seconds a frame at x100), the test checked between
frames, and it waited 120 simulated seconds for line contactors after a key that had not moved
the master controller at all. The rules that came out of it:

- **No SimulationRuntime in the tests.** `simulation_runtime_hook.gd` (libreloaded's, linked into
  `demo/tests/` by `make links`) places none: no frame moves SimulationServer's clock. The test
  steps it, tick by tick, as a step debugger does - `step(count)`,
  `wait_simulated_until(done, seconds, what)`, `ticks(seconds)`, `TICK` = 1/30 s (the simulation's
  tick to come, #301), of libreloaded's `ReloadedGutTest` (`demo/tests/reloaded_gut_test.gd`, a
  link), which `MaszynaGutTest` extends. Never `wait_seconds()`, never
  `wait_idle_frames()` or `Time.get_ticks_msec()` to let the simulation run, never a loop on
  `simulation_get_time()` awaiting frames (with no runtime it never ends).
- **Everything that moves with the simulation moves by simulation time** - the vehicles, the
  drivers, the events, and the cab's view of them: a widget's animation (libreloaded's
  `BaseCabinTool3D._process`) and a knob a key holds (`MaszynaCabinLogic`, on
  `simulation_advanced`). A test step moves them all; no
  frame rate (`--fixed-fps`) is needed or wanted.
- **A key press lasts at least one tick** (`MaszynaStartupTest.key_tap()`): the cab's logic takes
  keys on the simulation's step, a press that ends before the next tick is never seen.
- **A command or a key takes on the next tick, or it never will.** Check its own effect after one
  tick - the switch moved, the controller's position changed - and fail there
  (`MaszynaStartupTest._key_taken()`). Never fold "did the key work" into the wait for what it
  starts.
- **A physical process is waited for as long as the data says it takes**, in simulated seconds
  turned into ticks: a relay's `InitialCtrlDelay`, the doors' `open_delay + max_shift /
  open_speed`, the brake's `BDelay`, plus one `TICK` for the step that shows it. Never a blanket
  constant ("120 s a step", "20 s for the doors") and never a margin of a file's own (0.1, 0.25 -
  frame-clock numbers): the margin is `TICK`, from `ReloadedGutTest`.
- **A wait that runs out fails the test there**, saying what did not come about
  (`wait_simulated_until()` does). GUT's `wait_until()`/`wait_for_signal()` only return false -
  one whose result is ignored lets the test run on, or pass. An `await` of a signal with no limit
  hangs the run.
- **Loading is the machine's work, not simulated time**: a scenery fixture, a model, a screen.
  Its limit is real time and only a hang guard - a few seconds, named, the result checked (a
  fixture loads in under 0.1 s). Tens of seconds for it is the same antipattern.

## Physics is tested end to end and blackbox - ABSOLUTE

- A change to a physics component - the Mover wrapper (`src/maszyna/vehicles/`), a vehicle
  component (libreloaded's `src/vehicles/`, implemented here by the `MaszynaMover*` adapters), the
  FIZ factory (`addons/libmaszyna/fiz/`), the cab logic that commands them
  (`addons/libmaszyna/cabin/`) - is tested through the whole sequence a
  player goes through, not only the field that moved: battery -> low voltage
  (`power24_available`) -> pantographs -> relay reset -> main switch -> converter
  (`power110_available`) -> controller -> speed. A flag that is set (`battery_enabled`) proves
  nothing about what depends on it.
- Physics tests are **blackbox**: they assert on what components return (typed getters -
  `RailVehiclePowerSupply.get_power24_available()`, `RailVehicleEngine.get_main_switch_enabled()`)
  and/or on the state built into the dump (`get_state()`, `VehicleServer.vehicle_dump_state()`).
  Never on Mover internals, private members (`_name`) or the way a component got its value.
- Test the real entry point: commands through `vehicle_send_command`, cab controls through
  libreloaded's `CabinSystem.act` on a `MaszynaCabinLogic` - a hand-built struct can pass while the real path is
  broken.

## Data reading is tested on the data's variants - ABSOLUTE

A test of anything read from data (FIZ, MMD, SCN) carries a fixture with the variants the game data
really has, not only the commonest one: custom animation prefixes, `animations:` counts that stop
early, cab 2 declared before cab 1, a section left out. A fixture that only repeats the convention
proves nothing - the 36WE's pantograph names passed every test for a month.

## What a test may use

- Only fixtures: `demo/tests/fixtures/`, `demo/tests/materials/`. Never the game directory
  (`scenery/`, `dynamic/`, `textures/`) - CI has none. A real vehicle is cut into a fixture (FIZ,
  minimal MMD, the scenery lines it needs).
- Only the public interface of the tested class (`CODE_STYLE.md`, "Tests"). Needing private access
  means the API is missing something.
- Never a bare `[]`/`{}` to a parameter typed `Array[T]`/`Dictionary[K, V]` - the call is refused
  silently and the test checks nothing.
- A regression test is proven both ways: red on the code before the fix, green after.
- No throwaway diagnostic script in `demo/tests/` - probes live in the scratchpad.

## Running

- The test project is `demo/`, and the GUT pre-run hook and `ReloadedGutTest` are links to
  libreloaded's demo: `make links` first (every `make` target of a build or a run does it).
- Parse-check first: `godot-double --headless --path demo --check-only -s res://tests/<file>.gd`
  (5 s). A script that does not parse is skipped by GUT, `-gselect` then matches nothing and the
  whole directory runs until the timeout - it looks like a hang.
- Only the scripts you wrote or changed, **all in one process** - Godot's and GUT's start-up is
  paid once, not per script:
  `godot-double --headless --path demo --log-file <scratchpad>/godot.log -s addons/gut/gut_cmdln.gd -gconfig= -gpre_run_script=res://tests/simulation_runtime_hook.gd -gtest=res://tests/a.gd,res://tests/b.gd -gexit`.
  `-gconfig=` is required: `.gutconfig.json`'s `dirs` adds the whole directory to `-gtest`, which
  is why it once looked as if `-gtest` did not filter. Many scripts: two or three such processes
  side by side, the list split between them. One script: `make test TEST=<name>`
  (`-gtest=res://tests/<name>.gd`; it passes no `--log-file`).
  Never the whole suite.
- Redirect to a file and read the file - `| grep | head` kills the run with SIGPIPE.
- Every headless run takes `--log-file <scratchpad>/godot.log`: it shares the game's user
  directory, and its own `logs/app.log` rotates the operator's (five runs and it is gone).
- **A long timeout hides the early error.** A probe that hangs or fails in its third second, wrapped
  in `timeout 170`, says nothing for 170 s and then only "exit 124". Never one blocking call with a
  generous ceiling: the probe prints a line at every stage (`STAGE loaded`, `STAGE player in`, one
  per step), runs in the background into a file, and the file is watched - no new line for 15 s
  means it is hung or dead: kill it and read the file at once. The ceiling is the time the next
  stage should take, never the whole run "just in case".
- **A probe of a real scenery is capped at 60-120 s, whole run** (`timeout 120`), never more. It
  runs in the background; the first 15 s of its output are read for `Parse Error`/`SCRIPT ERROR`
  before anything waits on it (a probe that does not parse idles silently - 2026-10-10 a turn
  blocked 10 minutes on one). It prints its load progress (`load_progress`), so a long load is not
  silence. A long simulated span is reached by stepping more ticks per frame
  (`SimulationServer.simulation_advance(TICK)` N times between frames), never by a longer
  timeout. Never a tool call that sits in the turn waiting for a probe.
- A probe never `save_*`s a user setting - a killed probe leaves it written in the operator's
  `settings.cfg`. In memory only: `UserSettings.set_setting("maszyna", "game_dir", ...)` (libreloaded's
  `UserSettings`; `MaszynaGameDirectory` follows the setting).
- `godot-double`, never `godot` (double-precision extension).
- After every headless run `git status`: Godot rewrites `.tres`/`.tscn`; restore what you did not
  edit.

## Headless probe of a vehicle (scratchpad)

A probe reads frozen values - and "proves" anything - unless the vehicle is really simulated:

- it stands on a track: `RailVehiclePhysicsNode` plus a `RailVehicle3D` whose `controller_path`
  is set **before** it enters the tree (only `RailVehicleServer.vehicle_is_attached()` vehicles are
  stepped);
- the clock runs: a probe steps it itself (`SimulationServer.simulation_advance()`, as
  `ReloadedGutTest.step()` does) - with no `SimulationRuntime` nothing else moves it; check that
  `SimulationServer.simulation_get_time()` grows;
- it is driven (`PlayerServer.player_take_over_vehicle()`, or a driver at its controls -
  `DriverServer.vehicle_is_control_active()`), or a standing vehicle switches its physics off;
- waits are in simulated time the probe steps, never in frames or `create_timer` - a headless
  frame is microseconds and a real second says nothing of the simulation.

A probe using autoloads (`CabinSystem`, `UserSettings`) runs as a scene
(`godot-double --headless --audio-driver Dummy --path demo <scratch>/probe.tscn`); a `-s` script
does not compile against them. Without a wire the step feeds 0 V to the pantographs at its end - a
voltage fed by hand does not reach the cab's own tick. For a regression with no obvious cause,
build the commit before it in a copy in the scratchpad (`git archive <rev>^ | tar -x`, a copy of
libreloaded at the commit its `vendor/libreloaded` pinned then, `make libreloaded-local
LIBRELOADED_LOCAL=<that copy>`, `make compile-debug`) and run the same probe on both: the first step
that differs is the regression.

## Start-up test of a vehicle

Every locomotive and unit of the game data has a test that starts it from cold and moves it off
**by the player's keys** (`demo/tests/maszyna_startup_test.gd`, `MaszynaStartupTest`): the keys
go through libreloaded's `Player._unhandled_input` to the cab logic, every step is checked on the car it
acts on through component getters, and the first step that does not come about fails the test
with what the cars show (`start-up stopped at <step> after <n> s: ...`). The steps and the
original's conditions behind them are in `.claude/skills/driving-the-maszyna-vehicle/SKILL.md` -
read it before reading a red step as a bug.

- **Cut the vehicle into a fixture** (no models, sounds or textures; the FIZ, the MMD, every
  include and every member of a random include list, the skins' `.mat`):
  `scripts/cut-vehicle-fixture --game-dir ~/Games/Maszyna --dynamic pkp/<directory>` takes the
  directory's first vehicle with an engine and a cab - a multiple unit with all its cars, from the
  first scenery whose trainset it leads; `--scenery <file.scn> --vehicle <name>` takes a named
  vehicle's trainset. It writes `fixtures/scenery/startup_<name>.scn` on the start-up line
  (`startup_line.inc`, ep07.scn's kilometre of td.scn). Then `godot-double --headless --path demo
  --import` and commit the fixture with its `.fiz.import`.
- **One script per vehicle**, `demo/tests/test_zzz_startup_<directory>.gd`:

  ```gdscript
  extends MaszynaStartupTest

  func test_starts_and_moves_off() -> void:
      await run_startup("startup_<directory>.scn", "<vehicle name>", Kind.ELECTRIC_LOCOMOTIVE)
  ```

  `Kind` is how it starts (electric locomotive, electric or diesel unit, diesel-electric, diesel
  mechanical); the test fails when the powered car's engine or the unit type says otherwise.
  One script, one vehicle: `-gselect=test_zzz_startup_ep09_v1` runs it, `-gselect=test_zzz_startup`
  the whole batch (in the background, one script at a time).
- The test steps the clock at speed 1, tick by tick (see "The test steps the simulation"): every
  step's limit is the vehicle's own delay in ticks. The security system is acknowledged every
  frame, as a driver's reflex - a step is a frame.
- A red step is checked against the original (mover-parity-check) before anything is changed:
  either the driving is wrong (then the lesson goes to the driving skill) or the port is.

## Braking test of a vehicle

Every vehicle with a start-up test has a braking test (`demo/tests/maszyna_braking_test.gd`,
`MaszynaBrakingTest`, #310): started as its start-up test does it - which waits for the control
reservoirs to stop filling, as a driver does (an EStED's at 2.0 bar with the pipe at 4.9 brakes
nothing) - driven up to 40 km/h (half its top speed when lower) by the player's keys, a gearbox
shifting at full revs, the controller to 0, and stopped by one brake:

- `Brake.TRAIN` - the train brake handle at full service (`HANDLE_POSITION_FULL`), a
  `brake_level_increase` press at a time (held, an FV4a runs on to emergency);
- `Brake.EMERGENCY` - `brake_level_emergency` (Num 0, the original's trainbrakeemergency);
- `Brake.LOCAL` - the local brake in full by `local_brake_increase`. A vehicle whose local brake is
  the hand wheel or none, or an induction motor with an MHZ_EN57 handle (its local lever brakes
  nothing in the original), passes with a note; so does a vehicle with an individual brake (DL2,
  EL16) for the two train brake tests.

From the moment its cylinders reach 95% of the most they get, it has to stand within 1.5 times the
distance its data gives from the speed it has then: each braking vehicle's force at full pressure
(`RailVehicleBrake.get_force_at(1.0, v)`, `BrakeForceR`) - no more than its wheels' adhesion, the
rolling one or, when the brake is stronger, the sliding one (`Adhesive()`, Mover.cpp:4757-4763) -
against the trainset's mass. The local brake acts on the occupied car; an induction motor's is
pending until the ED/EP blending is ported (#350). How long the cylinders took
is logged, not asserted (`MASZYNA_ORIGINAL_QUIRKS.md`).

- **One script per vehicle**, `demo/tests/test_zzz_braking_<directory>.gd`, the start-up test's
  arguments with the brake added, a test function per brake (`test_the_train_brake_stops_it`,
  `test_the_emergency_brake_stops_it`, `test_the_local_brake_stops_it`).
- A red one is checked against the original (mover-parity-check) before the criterion is touched:
  the SM40's long stop was its wheels locking below 13 km/h, the original's physics.
