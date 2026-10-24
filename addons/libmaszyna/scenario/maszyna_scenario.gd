extends RefCounted
class_name MaszynaScenario

## The scenario of a loaded .scn scenery, running: its sounds audible, its `lua` scripts run and its
## keys taken (MaszynaScenarioKeyboard). The scenery only loads it (SceneryContentNode); its provider
## starts it once the scenery has loaded and stops it when the scenery unloads
## (MaszynaSceneryProvider.scenario_start(), scenario_stop()), so a scenery loaded without a game - in
## the editor - runs nothing.

## Where the scripts' relative paths start
const SCRIPTS_DIRECTORY:String = "scenery"

var _scenery_sounds:MaszynaScenerySounds = null
var _script_context:RID = RID()
## The scenario's keys, a child of the scenery while it runs
var _keyboard:MaszynaScenarioKeyboard = null


## The scripts run last, when everything a script may reach exists. The original runs them as it
## parses the file (simulationstateserializer.cpp:346-356); here the parsing runs on workers. Every
## scenery gets a context, so that a script can be applied to it while it runs. `progressed` is
## told the share of the start done (0..1) - the scenery's sounds made, the longest of it.
func start(scenery:SceneryContentNode, progressed:Callable = Callable()) -> void:
    # each step in the game's log before it runs: a crash with no message is found by the last one
    print("[ScenarioStart] building the scenery's sounds")
    _scenery_sounds = MaszynaSceneryProvider.scenery_get_sounds(scenery)
    await _scenery_sounds.build(progressed)
    print("[ScenarioStart] creating the script context")
    _script_context = ScenarioScriptServer.context_create(
            MaszynaGameDirectory.get_game_dir().path_join(SCRIPTS_DIRECTORY))
    ScenarioScriptServer.context_attach_cabin_implementation(_script_context, CabinScriptImplementation.new())
    for script_path:String in scenery.get_scenario_scripts():
        print("[ScenarioStart] running script %s" % script_path)
        ScenarioScriptServer.context_run_file(_script_context, script_path)
    _keyboard = MaszynaScenarioKeyboard.new()
    scenery.add_child(_keyboard)
    _keyboard.take_launchers()
    print("[ScenarioStart] started")


## The script context first, so no queued event of the scripts' own runs against what is freed
## after it
func stop() -> void:
    ScenarioScriptServer.context_free(_script_context)
    _script_context = RID()
    _scenery_sounds.clear()
    if _keyboard:
        _keyboard.queue_free()
        _keyboard = null


## The ScenarioScriptServer context the scenery's scripts run in, invalid while it does not run
func get_script_context() -> RID:
    return _script_context
