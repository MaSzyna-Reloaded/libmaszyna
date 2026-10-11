@tool
extends MaszynaIncludeNode
class_name MaszynaSceneryNode

## Emitted after every load with the first vehicle's train_id ("" when the scenery has none)
signal scenery_loaded(first_train_id:String)


@export var title:String = ""
@export var category:String = ""
@export_multiline var description:String = ""
## The environment the loaded scenery sets its time, date and weather in
@export_node_path("MaszynaEnvironmentNode") var environment_node_path:NodePath
@export_tool_button("Load") var load_action:Callable = self.load

## Original engine: clamp of scenario.weather.temperature (Globals.cpp:390)
const TEMPERATURE_MIN:float = -15.0
const TEMPERATURE_MAX:float = 45.0
## Original engine: clamp of a random overcast (simulationstateserializer.cpp:230)
const OVERCAST_MAX:float = 2.0
## Original engine: overcast up to 1 is the cloud cover, above it the weather turns to rain or snow
## (simulationenvironment.cpp:64-71) in two steps only - the light and the medium precipitation
## texture (opengl33precipitation.cpp:132). It has no storms, so the medium step stays at the
## precipitation the sky backend starts a storm from.
const OVERCAST_PRECIPITATION_START:float = 1.0
const OVERCAST_PRECIPITATION_MEDIUM:float = 1.35
const PRECIPITATION_LIGHT:float = 0.2
const PRECIPITATION_MEDIUM:float = 0.4
## Original engine: clamp of Global.fFogEnd (simulationstateserializer.cpp:216)
## The scenario clock of a scenery without a "time" section, 10:30 (scenario_time(),
## simulationtime.h:21 - Time.init() takes it after the load, simulationtime.cpp:30-51)
const START_TIME_DEFAULT:float = 10.5
const FOG_END_MIN:float = 10.0
const FOG_END_MAX:float = 25000.0

## train_id of the first vehicle found in the loaded scenery
var first_train_id:String = ""
## "time" section - scenario clock, hours; START_TIME_DEFAULT without one
var start_time:float = START_TIME_DEFAULT
## What else the loaded scenery declares about its environment; each *_defined says whether it does
var day_of_year_defined:bool = false
## "config movelight" - day of the year, 0 or less for today's date (simulationtime.cpp:45)
var day_of_year:int = 0
var temperature_defined:bool = false
## "config scenario.weather.temperature" - air temperature
var temperature:float = 0.0
var overcast_defined:bool = false
## "atmo" overcast - 0-1 cloud cover, above 1 precipitation (simulationenvironment.cpp:62)
var overcast:float = 0.0
var fog_end_defined:bool = false
## "atmo" fog range - the original's Global.fFogEnd, drawn from the declared range once per load
## like the original does (simulationstateserializer.cpp:216)
var fog_end:float = 0.0


func _enter_tree() -> void:
    super()
    # the editor keeps the environment saved with the edited scene (_load_content())
    if not Engine.is_editor_hint():
        ProjectSettings.settings_changed.connect(_apply_fog)


func _exit_tree() -> void:
    if not Engine.is_editor_hint():
        ProjectSettings.settings_changed.disconnect(_apply_fog)
    super()


func _clear_content(budget_msec:int = 0) -> void:
    await super._clear_content(budget_msec)
    first_train_id = ""
    start_time = START_TIME_DEFAULT
    day_of_year_defined = false
    temperature_defined = false
    overcast_defined = false
    fog_end_defined = false


func _load_content() -> void:
    await super._load_content()
    # the load was given up: nothing of it is announced
    if is_load_given_up():
        return
    first_train_id = _find_driver_train_id()
    _read_environment_declarations()
    # a scenery loaded in the editor must not rewrite the environment saved with the edited scene
    if not Engine.is_editor_hint():
        _apply_environment_declarations()
    scenery_loaded.emit(first_train_id)


## A later section overrides an earlier one, like the original parsing the file top to bottom
func _read_environment_declarations() -> void:
    for node:Node in find_children("", "MaszynaTimeNode", true, false):
        start_time = (node as MaszynaTimeNode).start_time
    for node:Node in find_children("", "MaszynaConfigNode", true, false):
        var values:Dictionary[String, String] = (node as MaszynaConfigNode).values
        if values.has(MaszynaConfigNode.KEY_DAY_OF_YEAR):
            day_of_year_defined = true
            day_of_year = values[MaszynaConfigNode.KEY_DAY_OF_YEAR].to_int()
        if values.has(MaszynaConfigNode.KEY_TEMPERATURE):
            temperature_defined = true
            temperature = clampf(
                values[MaszynaConfigNode.KEY_TEMPERATURE].to_float(), TEMPERATURE_MIN, TEMPERATURE_MAX)
    for node:Node in find_children("", "MaszynaAtmoNode", true, false):
        var atmo:MaszynaAtmoNode = node as MaszynaAtmoNode
        fog_end_defined = true
        fog_end = clampf(
            randf_range(
                minf(atmo.fog_range_start, atmo.fog_range_end), maxf(atmo.fog_range_start, atmo.fog_range_end)),
            FOG_END_MIN, FOG_END_MAX)
        if atmo.overcast_defined:
            overcast_defined = true
            # negative overcast means a random value in range 0-abs(specified range)
            # (simulationstateserializer.cpp:226)
            overcast = (
                atmo.overcast if atmo.overcast >= 0.0
                else randf_range(0.0, minf(absf(atmo.overcast), OVERCAST_MAX)))


func _apply_environment_declarations() -> void:
    var environment_node:MaszynaEnvironmentNode = get_node_or_null(environment_node_path) as MaszynaEnvironmentNode
    if not environment_node:
        return
    environment_node.use_system_time = false
    environment_node.current_time = start_time
    if day_of_year_defined:
        var date:Dictionary = (
            Time.get_date_dict_from_system() if day_of_year <= 0
            else _date_from_day_of_year(day_of_year, environment_node.year))
        environment_node.use_system_time = false
        environment_node.day = date["day"]
        environment_node.month = date["month"]
    if temperature_defined:
        environment_node.temperature = temperature
    _apply_fog()
    if overcast_defined:
        environment_node.cloudiness = clampf(overcast, 0.0, 1.0)
        environment_node.precipitation = _precipitation_from_overcast(overcast)


## The scenery's fog, with FOG_SCENERY_DISTANCE_FACTOR_SETTING as it is now - applied again when the
## setting changes while the scenery stands
func _apply_fog() -> void:
    var environment_node:MaszynaEnvironmentNode = get_node_or_null(environment_node_path) as MaszynaEnvironmentNode
    if not fog_end_defined or not environment_node:
        return
    # Original engine: the fog has no density of its own, its range is
    # fFogEnd / max(1, Overcast * 2) (opengl33renderer.cpp:4685); see FOG_CURVE_SETTING for how
    # that range becomes the distance of a complete fog
    var fog_range:float = fog_end / maxf(1.0, overcast * 2.0 if overcast_defined else 0.0)
    environment_node.fog_distance = fog_range * float(ProjectSettings.get_setting(
        MaszynaSkyEnvironment.FOG_SCENERY_DISTANCE_FACTOR_SETTING,
        MaszynaSkyEnvironment.FOG_SCENERY_DISTANCE_FACTOR_DEFAULT))
    environment_node.fog_density = 1.0


func _precipitation_from_overcast(value:float) -> float:
    if value <= OVERCAST_PRECIPITATION_START:
        return 0.0
    if value < OVERCAST_PRECIPITATION_MEDIUM:
        return PRECIPITATION_LIGHT
    return PRECIPITATION_MEDIUM


func _date_from_day_of_year(year_day:int, year:int) -> Dictionary:
    var new_year:int = Time.get_unix_time_from_datetime_dict({"year": year, "month": 1, "day": 1})
    return Time.get_date_dict_from_unix_time(new_year + (year_day - 1) * LibMaszynaUnits.SECONDS_PER_DAY)


## The player belongs in a vehicle with a driver, not in whatever vehicle the scenery declares
## first (MaszynaDynamicData.driver_type)
func _find_driver_train_id() -> String:
    var reverse_driver_train_id:String = ""
    for vehicle:RID in get_vehicles():
        var dynamic:MaszynaDynamicData = MaszynaLegacyVehicleSystem.vehicle_get_dynamic(vehicle)
        if dynamic.driver_type == MaszynaDynamicData.DriverType.DRIVER_HEAD:
            return dynamic.name
        if dynamic.driver_type == MaszynaDynamicData.DriverType.DRIVER_REAR and not reverse_driver_train_id:
            reverse_driver_train_id = dynamic.name
    if reverse_driver_train_id:
        return reverse_driver_train_id
    return MaszynaLegacyVehicleSystem.vehicle_get_dynamic(get_vehicles()[0]).name if get_vehicles() else ""
