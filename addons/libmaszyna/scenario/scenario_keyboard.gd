extends Node
class_name ScenarioKeyboard

## The scenario's keys. Shift+0..9 queue the scenery's `keyctrl00`..`keyctrl09` events
## (drivermode.cpp:446-455, 907-917); an event launcher bound to a key fires when the key is
## pressed within its radius of the camera, its second event with Shift (TEventLauncher,
## scene.cpp:691-705).

## The input actions of Shift+0..Shift+9, in their order
const KEYCTRL_ACTIONS:Array[StringName] = [
    &"scenario_keyctrl_0", &"scenario_keyctrl_1", &"scenario_keyctrl_2", &"scenario_keyctrl_3",
    &"scenario_keyctrl_4", &"scenario_keyctrl_5", &"scenario_keyctrl_6", &"scenario_keyctrl_7",
    &"scenario_keyctrl_8", &"scenario_keyctrl_9",
]
## The event of the n-th action (drivermode.cpp:450)
const KEYCTRL_EVENT:String = "keyctrl%02d"

## Launchers by the key that fires them, taken when a scenery has loaded
var _launchers_by_key:Dictionary[Key, Array] = {}


## Takes the launchers of the loaded scenery (connected to its `scenery_loaded` in the scene)
func take_launchers(_first_train_id:String = "") -> void:
    _launchers_by_key.clear()
    for launcher:RID in ScenarioEventServer.launcher_get_rids():
        var key:Key = ScenarioEventServer.launcher_get_key(launcher)
        if key == KEY_NONE:
            continue
        if not _launchers_by_key.has(key):
            _launchers_by_key[key] = []
        _launchers_by_key[key].append(launcher)


func _unhandled_input(event:InputEvent) -> void:
    for index:int in KEYCTRL_ACTIONS.size():
        if event.is_action_pressed(KEYCTRL_ACTIONS[index], false, true):
            var keyctrl:RID = ScenarioEventServer.event_get_rid_by_name(KEYCTRL_EVENT % index)
            if keyctrl.is_valid():
                ScenarioEventServer.event_queue(keyctrl)
            get_viewport().set_input_as_handled()
            return

    var key_event:InputEventKey = event as InputEventKey
    if not key_event or not key_event.pressed or key_event.echo:
        return
    if not _launchers_by_key.has(key_event.keycode):
        return
    var camera:Camera3D = get_viewport().get_camera_3d()
    for launcher:RID in _launchers_by_key[key_event.keycode]:
        var radius:float = ScenarioEventServer.launcher_get_radius(launcher)
        var in_range:bool = radius < 0.0 or (
            camera and camera.global_position.distance_to(ScenarioEventServer.launcher_get_position(launcher)) <= radius
        )
        if not in_range:
            continue
        if key_event.shift_pressed:
            ScenarioEventServer.launcher_fire_shift(launcher)
        else:
            ScenarioEventServer.launcher_fire(launcher)
