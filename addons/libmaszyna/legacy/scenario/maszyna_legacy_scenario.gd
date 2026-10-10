extends RefCounted
class_name MaszynaLegacyScenario

## The scenario of a loaded scenery, running: its sounds audible and its `lua` scripts run. The
## scenery only loads it (MaszynaIncludeNode); the game starts it once the scenery has loaded and
## stops it when the scenery unloads (MaszynaIncludeNode.unloading), so a scenery loaded without a
## game - in the editor - runs nothing.

## Where the scripts' relative paths start
const SCRIPTS_DIRECTORY:String = "scenery"

var _scenery_sounds:MaszynaLegacyScenerySounds = null
var _script_context:RID = RID()


## The scripts run last, when everything a script may reach exists. The original runs them as it
## parses the file (simulationstateserializer.cpp:346-356); here the parsing runs on workers. Every
## scenery gets a context, so that a script can be applied to it while it runs. `progressed` is
## told the share of the start done (0..1) - the scenery's sounds made, the longest of it.
func start(scenery:MaszynaIncludeNode, progressed:Callable = Callable()) -> void:
    # each step in the game's log before it runs: a crash with no message is found by the last one
    print("[ScenarioStart] building the scenery's sounds")
    _scenery_sounds = scenery.get_scenery_sounds()
    await _scenery_sounds.build(progressed)
    print("[ScenarioStart] creating the script context")
    _script_context = ScenarioScriptServer.context_create(
            UserSettings.get_maszyna_game_dir().path_join(SCRIPTS_DIRECTORY))
    ScenarioScriptServer.context_attach_cabin_implementation(_script_context, CabinScriptImplementation.new())
    for script_path:String in scenery.get_scenario_scripts():
        print("[ScenarioStart] running script %s" % script_path)
        ScenarioScriptServer.context_run_file(_script_context, script_path)
    print("[ScenarioStart] started")


## The script context first, so no queued event of the scripts' own runs against what is freed
## after it
func stop() -> void:
    ScenarioScriptServer.context_free(_script_context)
    _script_context = RID()
    _scenery_sounds.clear()


## The ScenarioScriptServer context the scenery's scripts run in, invalid while it does not run
func get_script_context() -> RID:
    return _script_context
