extends Control

var _t:float = 0.0

@onready var train: RailVehiclePhysicsNode = $SM42
@onready var battery_progress_bar = $%BatteryProgressBar

const DEBUG_PANEL: PackedScene = preload("res://addons/libmaszyna/debug_hud/panel.tscn")

## The vehicle, and one panel for each of its components, made once it stands
## (_on_sm_42_vehicle_changed())
var _vehicle: RID = RID()
## The driver sitting in the vehicle's front cab - the Mover activates only an occupied cab
## (CabActivisation(), Mover.cpp:2658), switched on by the "Cab" switch; a person outlives a
## vehicle, so it goes with the demo (_exit_tree())
var _driver: RID = RID()
var _panels: Dictionary[VehicleComponent, DebugPanel] = {}

@onready var FORWARD = %MoverSwitches/Controller/Direction/Forward
@onready var REVERSE = %MoverSwitches/Controller/Direction/Reverse

const rich_print_loglevel_colors = {
    GameLog.LogLevel.DEBUG: "#777",
    GameLog.LogLevel.ERROR: "red",
    GameLog.LogLevel.WARNING: "orange",
    }

const loglevel_names = {
    GameLog.LogLevel.DEBUG: "DEBUG",
    GameLog.LogLevel.INFO: "INFO",
    GameLog.LogLevel.WARNING: "WARNING",
    GameLog.LogLevel.ERROR: "ERROR",
    }

const CONSOLE_LOG_HANDLER := "mover_demo_console"


## The lines of the game's logger, to the Godot console
class ConsoleLogHandler extends GameLogHandler:
    var _print_entry: Callable

    func _init(print_entry: Callable) -> void:
        _print_entry = print_entry

    func _handle(_logger_id: String, loglevel: GameLog.LogLevel, line: String) -> void:
        _print_entry.call(loglevel, line)


# Called when the node enters the scene tree for the first time.
func _ready() -> void:
    $%TrainName.text = tr("%s (type: %s)") % [train.vehicle_id, train.controller.type_name]
    GameLog.register_handler(CONSOLE_LOG_HANDLER, ConsoleLogHandler.new(print_log_entry_to_godot_console))
    GameLog.assign_handler("game", CONSOLE_LOG_HANDLER)


func _exit_tree() -> void:
    GameLog.unassign_handler("game", CONSOLE_LOG_HANDLER)
    GameLog.unregister_handler(CONSOLE_LOG_HANDLER)
    PersonServer.person_free(_driver)


func _colorize_loglevel(loglevel, line):
    var color = rich_print_loglevel_colors.get(loglevel)
    if color:
        return "[color=%s]%s[/color]" % [color, line]
    else:
        return line

func print_log_entry_to_godot_console(loglevel, line):
    print_rich(_colorize_loglevel(loglevel, "%s: %s" % [loglevel_names[loglevel], line]))


func print_train_log_entry_to_godot_console(train, loglevel, line):
    print_rich(_colorize_loglevel(loglevel, "LOG: [%s][%s] %s" % [train, loglevel, line]))

func draw_dictionary(dict: Dictionary, target: DebugPanel):
    if not dict:
        target.text = ""
        return
    var lines = []
    for k in dict.keys():
        lines.append("%s=%s" % [k, dict[k]])
    target.text = "\n".join(lines)

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
    _t += delta

    if(_t>0.1) and _vehicle.is_valid():
        _t = 0

        var train_state = VehicleServer.vehicle_dump_state(_vehicle)
        var bv = train_state.get("battery_voltage", 0)

        $%BatteryProgressBar.value = bv
        $%BatteryValue.text = "%.2f V" % [bv]

        for component: VehicleComponent in _panels:
            draw_dictionary(component.get_state(), _panels[component])



func _on_brake_level_value_changed(value):
    VehicleServer.vehicle_broadcast_command("brake_level_set", value, null)

func _on_main_decrease_button_up():
    VehicleServer.vehicle_send_command(_vehicle, "main_controller_decrease")

func _on_main_increase_button_up():
    VehicleServer.vehicle_send_command(_vehicle, "main_controller_increase")

func _on_reverse_button_up():
    VehicleServer.vehicle_send_command(_vehicle, "direction_decrease")

func _on_forward_button_up():
    VehicleServer.vehicle_send_command(_vehicle, "direction_increase")

## The vehicle stands with its controller (VehiclePhysicsNode.vehicle_changed)
func _on_sm_42_vehicle_changed():
    _vehicle = $SM42.get_vehicle_rid()
    for panel: DebugPanel in _panels.values():
        panel.queue_free()
    _panels.clear()
    for component: VehicleComponent in VehicleServer.vehicle_get_controller(_vehicle).get_components():
        var panel: DebugPanel = DEBUG_PANEL.instantiate()
        panel.title = component.get_class()
        %DebugPanels.add_child(panel)
        _panels[component] = panel
    RailVehicleServer.vehicle_add_front_cabin(_vehicle)
    RailVehicleServer.vehicle_add_rear_cabin(_vehicle)
    _driver = PersonServer.person_create()
    RailVehicleServer.person_enter_front_cabin(_driver, _vehicle, VehiclePersonRole.VEHICLE_PERSON_ROLE_DRIVER)
    %MoverSwitches.vehicle = _vehicle
    print("Mover initialized. Train config: ", VehicleServer.vehicle_dump_config(_vehicle))
