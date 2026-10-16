extends VBoxContainer
class_name MaszynaStationsPanel

## The stations of a world of the game's own format (MaszynaStationData of its scenery,
## MaszynaCompiledScenery.stations): a list of them by name and code, and Show - the free camera
## looking at the station picked from SHOW_DISTANCE south of it, SHOW_HEIGHT up
## (PlayerCameraServer.camera_place()). Shown in the HUD (DebugHud.attach_world()); filled once the
## world's scenery is loaded.

## Where the camera looks at a station from: this far south of it, this high over it [m]
const SHOW_DISTANCE:float = 250.0
const SHOW_HEIGHT:float = 150.0

var _world:MaszynaWorld = null
var _stations:Array[MaszynaStationData] = []


## The world whose stations are listed; null for none
func attach_world(world:MaszynaWorld) -> void:
    if _world:
        _world.scenery_loaded.disconnect(_on_scenery_loaded)
    _world = world
    if _world:
        _world.scenery_loaded.connect(_on_scenery_loaded)
    _fill()


func _on_scenery_loaded(_first_vehicle_id:String) -> void:
    _fill()


func _fill() -> void:
    _stations.clear()
    if _world and _world.scenery:
        _stations.assign(_world.scenery.stations)
    _stations.sort_custom(func(a:MaszynaStationData, b:MaszynaStationData) -> bool:
            return a.station_name.naturalnocasecmp_to(b.station_name) < 0)
    %StationList.clear()
    for station:MaszynaStationData in _stations:
        %StationList.add_item("%s (%s)" % [station.station_name, station.ref] if station.ref else station.station_name)
    %Show.disabled = _stations.is_empty()


## The free camera looking at the station picked in the list
func show_selected() -> void:
    var picked:PackedInt32Array = %StationList.get_selected_items()
    if not picked:
        return
    var target:Vector3 = _stations[picked[0]].position
    var eye:Vector3 = target + Vector3(0.0, SHOW_HEIGHT, SHOW_DISTANCE)
    PlayerCameraServer.camera_place(Transform3D(Basis.looking_at(target - eye), eye))
