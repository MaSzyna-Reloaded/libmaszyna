@tool
extends RefCounted
class_name MaszynaDestinationSigns

## The vehicles' destination signs - the submodel of replaceable skin 4 (TDynamicObject::DestinationSign,
## DynObj.cpp:2599, 3108-3130, 7869-7962). The sign shows, as slot 4 of the vehicle's skin:
## - its train's: a texture of the vehicle's directory named after the train's number, then after its
##   destination - "<name>@<type>" before "<name>" -, else the `make:` texture its `pydestinationsign:`
##   script draws for the train (MaszynaMaterialManager: in Lua when a `.lua` file stands beside the
##   script, else in Python);
## - its background (`background:`, "nowhere" without one).
## A skin of four slots keeps its own slot 4, whatever the timetable (DynObj.cpp:3120, 7878) - a texture
## drawn under a name (MaszynaTextureLuaNode) is shown there as the skin's fourth name.
## A sign that shines (its submodel self-illuminated) shows its train's only with low voltage on and a
## driver in its trainset, and shines then; a physical one shows its train's whenever it has one.
## Each vehicle's sign is its own - the part's material of its exterior's instance, set again whenever
## the exterior is built anew.

## A sign's light threshold, shining and dark (fLight 2.0 and -1.0, DynObj.cpp:3114-3117)
const LIT_THRESHOLD:float = 2.0
const DARK_THRESHOLD:float = -1.0
## The background of a sign whose MMD names none (DynObj.cpp:7051)
const DEFAULT_BACKGROUND:String = "nowhere"
## The skins of a vehicle whose sign the timetable does not change (multi_textures, DynObj.cpp:3120)
const FIXED_SIGN_SKIN_COUNT:int = 4
## The sign's skin, replaceable skin 4 (replacable_skins[4], DynObj.cpp:3121)
const SIGN_SKIN:int = MaszynaRailVehicle3DInstancer.DESTINATION_SIGN_SKIN
## The files a sign's texture is found in, as MaszynaMaterialManager reads them
const TEXTURE_EXTENSIONS:PackedStringArray = [".mat", ".dds"]
const TEXTURES_DIRECTORY:String = "textures"

## A vehicle's sign
class Sign:
    var vehicle:RID
    ## The vehicle's directory with a leading slash, its .mmd's base name and what its structure says of
    ## its sign (MaszynaVehicleMmdData.destination_sign)
    var data_path:String
    var type_name:String
    var definition:Dictionary
    ## Its skin fills every replaceable slot: the sign is the skin's own
    var fixed:bool = false
    ## Whether it has low voltage (RailVehiclePowerSupply.power_changed)
    var powered:bool = false
    ## Its train's sign, a material's name; none without a train
    var train_material:String = ""


var _signs:Dictionary[RID, Sign] = {}


func _init() -> void:
    RailVehicleRenderingServer.vehicle_model_built.connect(_on_vehicle_model_built)
    RailVehicleServer.trainset_vehicles_changed.connect(_on_trainset_vehicles_changed)
    StationServer.run_created.connect(_on_run_created)
    StationServer.run_freed.connect(_on_run_freed)


## The vehicle's sign, once it is built: `definition` is its structure's destination_sign, `skin_count`
## how many replaceable skins its skin has
func vehicle_add(vehicle:RID, data_path:String, type_name:String, definition:Dictionary, skin_count:int) -> void:
    var sign := Sign.new()
    sign.vehicle = vehicle
    sign.data_path = data_path
    sign.type_name = type_name
    sign.definition = definition
    sign.fixed = skin_count >= FIXED_SIGN_SKIN_COUNT
    _signs[vehicle] = sign
    var power:RailVehiclePowerSupply = RailVehicleServer.vehicle_component_get(
            vehicle, RailVehicleComponentType.COMPONENT_POWER_SUPPLY) as RailVehiclePowerSupply
    if power:
        sign.powered = power.get_power24_available() or power.get_power110_available()
        power.power_changed.connect(_on_power_changed.bind(vehicle))
    _set_train(sign)


func vehicle_remove(vehicle:RID) -> void:
    _signs.erase(vehicle)


## The train the vehicle runs in: its trainset's run (DestinationSet(Relation2, TrainName),
## Driver.cpp:2494, 4563-4568)
func _set_train(sign:Sign) -> void:
    sign.train_material = ""
    if sign.fixed:
        _show(sign)
        return
    var trainset:RID = RailVehicleServer.vehicle_get_trainset(sign.vehicle)
    var run:RID = StationServer.run_find_by_trainset(trainset) if trainset.is_valid() else RID()
    var timetable:RID = StationServer.run_get_timetable(run) if run.is_valid() else RID()
    var number:String = StationServer.timetable_get_train_name(timetable) if timetable.is_valid() else PythonScreenState.NO_TRAIN
    var station:RID = StationServer.timetable_get_destination(timetable) if timetable.is_valid() else RID()
    var destination:String = StationServer.station_get_name(station) if station.is_valid() else ""
    for name:String in [number, destination]:
        sign.train_material = _find_texture(sign, name)
        if sign.train_material:
            break
    var script:String = sign.definition.get("script", "")
    if not sign.train_material and script and not number == PythonScreenState.NO_TRAIN:
        sign.train_material = _train_sign_request(sign, script, number)
    _show(sign)


## A texture of the vehicle's directory named after `name`, its Polish letters plain
## (DestinationFind(), Bezogonkow(), DynObj.cpp:7929-7962): the name for the vehicle's type first
func _find_texture(sign:Sign, name:String) -> String:
    if not name:
        return ""
    if name.begins_with(MaszynaMaterialManager.GENERATED_TEXTURE_SCHEME):
        return name
    var plain:String = Windows1250.to_ascii(name).to_lower()
    var game_dir:String = MaszynaGameDirectory.get_game_dir()
    var model_path:String = sign.data_path.trim_prefix("/")
    for candidate:String in [plain + "@" + sign.type_name.to_lower(), plain]:
        for path:String in [model_path.path_join(candidate), candidate, TEXTURES_DIRECTORY.path_join(candidate)]:
            for extension:String in TEXTURE_EXTENSIONS:
                if FileAccess.file_exists(game_dir.path_join(MaszynaDataPath.resolve(game_dir, path + extension))):
                    return candidate
    return ""


## The `make:` request of the sign its script draws for the train (DestinationSet(),
## DynObj.cpp:7895-7926): it names the timetable's vehicle - the one its driver sits in -, the train
## and the vehicle's instance
func _train_sign_request(sign:Sign, script:String, number:String) -> String:
    var parameters:String = sign.definition.get("parameters", "")
    var instancing:String = sign.definition.get("instancing", "")
    var request:String = "%s%s?%s=%s&_id1=%s" % [
            MaszynaMaterialManager.GENERATED_TEXTURE_SCHEME, script, MaszynaMaterialManager.TIMETABLE_KEY,
            VehicleServer.vehicle_get_name(_timetable_vehicle(sign.vehicle)), number]
    if instancing:
        request += "&_id2=" + (
                VehicleServer.vehicle_get_name(sign.vehicle) if instancing == "name"
                else sign.type_name if instancing == "type" else PythonScreenState.NO_TRAIN)
    if parameters:
        request += "&" + parameters
    return request


## The vehicle whose driver runs the trainset (ctOwner, Driver.cpp:2465); the vehicle itself without one
func _timetable_vehicle(vehicle:RID) -> RID:
    var trainset:RID = RailVehicleServer.vehicle_get_trainset(vehicle)
    if trainset.is_valid():
        for member:RID in RailVehicleServer.trainset_get_vehicles(trainset):
            if DriverServer.vehicle_get_driver(member).is_valid():
                return member
    return vehicle


## Whether a driver runs the vehicle's trainset (ctOwner != nullptr, DynObj.cpp:3125) - a carriage left
## on a siding has none
func _has_driver(vehicle:RID) -> bool:
    return DriverServer.vehicle_get_driver(_timetable_vehicle(vehicle)).is_valid()


## update_destinations(), DynObj.cpp:3108-3130: the light by the power, then - for a sign the timetable
## changes - its train's or its background as the skin's slot 4
func _show(sign:Sign) -> void:
    var model:RID = RailVehicleRenderingServer.vehicle_get_model(sign.vehicle)
    if not model.is_valid():
        return
    var submodel:String = sign.definition["submodel"]
    var lit:bool = sign.definition.get("lit", false)
    if lit:
        MaszynaE3DRenderingServer.instance_set_part_lights_on_threshold(
                model, submodel, LIT_THRESHOLD if sign.powered else DARK_THRESHOLD)
    if sign.fixed:
        return
    var shown:String = (sign.train_material
            if sign.train_material and (not lit or (sign.powered and _has_driver(sign.vehicle)))
            else sign.definition.get("background", DEFAULT_BACKGROUND))
    # set only when it changes: the skin builds the model again, and a model built shows its sign
    var skins:PackedStringArray = MaszynaE3DRenderingServer.instance_get_skins(model)
    if skins.size() <= SIGN_SKIN or not skins[SIGN_SKIN] == shown:
        MaszynaE3DRenderingServer.instance_set_skin(model, SIGN_SKIN, shown)


func _on_vehicle_model_built(vehicle:RID) -> void:
    if _signs.has(vehicle):
        _show(_signs[vehicle])


func _on_power_changed(is_powered:bool, vehicle:RID) -> void:
    var sign:Sign = _signs.get(vehicle)
    if sign:
        sign.powered = is_powered
        _show(sign)


## Its vehicles are others now, a trainset just made included: each of them looks for its train again
func _on_trainset_vehicles_changed(trainset:RID) -> void:
    for vehicle:RID in RailVehicleServer.trainset_get_vehicles(trainset):
        if _signs.has(vehicle):
            _set_train(_signs[vehicle])


func _on_run_created(run:RID) -> void:
    for vehicle:RID in RailVehicleServer.trainset_get_vehicles(StationServer.run_get_trainset(run)):
        if _signs.has(vehicle):
            _set_train(_signs[vehicle])


## Announced once the run is gone, its trainset with it: every sign looks for its train again
func _on_run_freed(_run:RID) -> void:
    for sign:Sign in _signs.values():
        _set_train(sign)
