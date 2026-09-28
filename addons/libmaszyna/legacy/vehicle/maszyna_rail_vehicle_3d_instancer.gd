@tool
extends RefCounted
class_name MaszynaRailVehicle3DInstancer

## Reads what a vehicle's MMD says it is built from, out of nothing but a
## data_path/file_name/skin triple: its appearance (the models and the submodels that move, drawn
## by RailVehicleRenderingServer), the models of its cargo and its interactive MMD-driven cabin
## (CabinSystem). Used by MaszynaLegacyVehicleSystem, which builds the vehicle from it.

## The bogies, by the names the original itself looks for (DynObj.cpp:2482-2487). Everything else
## that moves - wheels, pantographs, wipers, mirrors - is named by the vehicle's MMD
## (MmdCabinInstancer.parse_*_names).
const FRONT_BOGIE_SUBMODEL_NAMES:Array[String] = ["bogie1", "boogie01"]
const REAR_BOGIE_SUBMODEL_NAMES:Array[String] = ["bogie2", "boogie02"]
## The elements of a wiper (DynObj.cpp:5845-5866)
const WIPER_ELEMENT_SUFFIXES:Array[String] = ["_p1", "_p2", "_p3"]

## The pantograph element without which the pantograph is not animated: the slider (DynObj.cpp:5562-5566;
## an index of MmdCabinInstancer.PANTOGRAPH_ELEMENT_LABELS)
const PANTOGRAPH_REQUIRED_ARMS:Array[int] = [4]
## The front and the rear pantograph (Pantographs[end::front/rear], MOVER.h)
## An axle's diameter: the front rolling, the powered and the rear rolling one (dWheelAngle[0..2],
## DynObj.cpp:5362-5385)
## The destination sign's replaceable skin: material -4, held as its index 3 (E3DModelBuilder.cpp:153)
const DESTINATION_SIGN_SKIN:int = 3
enum WheelGroup { FRONT_ROLLING, POWERED, REAR_ROLLING }
## The axle arrangement's symbols of powered axles, A to J, and of rolling ones, 1 to 9
## (DynObj.cpp:5372, 5378)
const POWERED_AXLES_FIRST:int = 65
const POWERED_AXLES_LAST:int = 74
const ROLLING_AXLES_FIRST:int = 49
const ROLLING_AXLES_LAST:int = 57
const PANTOGRAPH_COUNT:int = 2

## Every MaSzyna-authored piece of a vehicle (exterior, low-poly interior, passengers, cab) lives in
## one vehicle-local frame where +Z is the direction of travel: the original draws all of them under
## the same TDynamicObject::mMatrix, built by BasisChange(vLeft, vUp, vFront) (DynObj.cpp:2506-2508;
## opengl33renderer.cpp:1174, 2856, 2976). A vehicle faces -Z here, so each of them is turned about
## its vertical by half a turn.
const MASZYNA_VEHICLE_FRAME:Transform3D = Transform3D(Basis(Vector3.UP, PI), Vector3.ZERO)


## Reads what the vehicle's MMD says it is built from. This is the expensive half - every call
## opens and re-parses the MMD and loads the exterior model - and its result is what
## MaszynaLegacyVehicleSystem caches.
static func read_structure(
        data_path:String, file_name:String, skin:String, vehicle_name:String) -> MaszynaVehicleStructure:
    if not data_path or not file_name:
        return null

    # MaterialManager.get_submodel_material() builds its material-search path by dropping
    # data_path's FIRST "/"-separated segment - meant to strip the artifact empty segment from a
    # LEADING slash, not the "dynamic" directory name itself. Every hand-authored vehicle scene
    # except su45 (whose skin is consequently broken the same way) uses a leading slash
    # (e.g. "/dynamic/pkp/ep09_v1/") for exactly this reason. Normalize here so operators don't
    # need to know about this quirk.
    var normalized_data_path:String = data_path if data_path.begins_with("/") else "/" + data_path
    var game_dir:String = UserSettings.get_maszyna_game_dir()
    var relative_mmd_path:String = normalized_data_path.trim_prefix("/").path_join(file_name + ".mmd")
    var abs_mmd_path:String = game_dir.path_join(MaszynaDataPath.resolve(game_dir, relative_mmd_path))
    var parameters:Dictionary = MmdCabinInstancer.vehicle_parameters(vehicle_name, file_name, skin)

    var structure := MaszynaVehicleStructure.new()
    structure.data_path = normalized_data_path
    structure.file_name = file_name
    var appearance := RailVehicleAppearance.new()
    appearance.data_path = normalized_data_path
    appearance.model_transform = MASZYNA_VEHICLE_FRAME
    # The exterior body model filename is NOT the same as file_name in general (confirmed
    # against real data: dynamic/pkp/st44_v2's body model isn't named after its .fiz/.mmd base) -
    # it comes from the MMD's own top-level "models:" line. Fall back to file_name only if that
    # can't be read, rather than silently building a vehicle with no model at all.
    var body_model_filename:String = MmdCabinInstancer.parse_body_model(abs_mmd_path, parameters)
    if not body_model_filename:
        body_model_filename = file_name
    appearance.model_filename = body_model_filename

    var lowpoly_filename:String = MmdCabinInstancer.parse_lowpoly_interior_model(abs_mmd_path, parameters)
    if lowpoly_filename:
        appearance.low_poly_model_filename = lowpoly_filename

    appearance.attachment_model_filenames = MmdCabinInstancer.parse_attachments(abs_mmd_path, parameters)

    structure.load_models = MmdCabinInstancer.parse_loads(abs_mmd_path, parameters)
    var passengers_filename:String = structure.load_models.get("passengers", "")
    if passengers_filename:
        appearance.passengers_model_filename = passengers_filename

    appearance.skins = PackedStringArray(MmdCabinInstancer.resolve_skins(normalized_data_path, skin))
    appearance.joint_cabs = MmdCabinInstancer.parse_joint_cabs(abs_mmd_path, parameters)
    var model:E3DModel = E3DModelManager.load_model(normalized_data_path, appearance.model_filename)
    if model:
        # the wheels' diameters and axle arrangement split the axles into powered and rolling ones
        var description:VehicleController = FizVehicleBuilder.build_description(
                normalized_data_path.trim_prefix("/"), file_name)
        var wheels:RailVehicleWheels = null
        if description:
            for component:VehicleComponent in description.get_components():
                if component is RailVehicleWheels:
                    wheels = component
        _resolve_parts(appearance, model, wheels, MmdCabinInstancer.parse_wheel_names(abs_mmd_path, parameters),
                MmdCabinInstancer.parse_pantograph_element_names(abs_mmd_path, parameters),
                MmdCabinInstancer.parse_wiper_names(abs_mmd_path, parameters),
                MmdCabinInstancer.parse_mirror_names(abs_mmd_path, parameters),
                MmdCabinInstancer.parse_door_names(abs_mmd_path, parameters),
                MmdCabinInstancer.parse_door_step_names(abs_mmd_path, parameters),
                MmdCabinInstancer.parse_pendulums(abs_mmd_path, parameters),
                MmdCabinInstancer.parse_pantograph_factors(abs_mmd_path, parameters))
    structure.appearance = appearance
    structure.coupler_adapter = MmdCabinInstancer.parse_coupler_adapter(abs_mmd_path, parameters)
    structure.cabin_scene = _build_cabin_scene(normalized_data_path, file_name, skin)
    structure.cabin_kinds = MmdCabinInstancer.parse_cabin_kinds(abs_mmd_path, parameters)
    return structure


## Which model a cargo is drawn as, in the order the original tries them
## (TDynamicObject::LoadMMediaFile_mdload(), DynObj.cpp:7195): the vehicle's own override for that
## cargo, then a model named for this vehicle and the cargo together, then one named after the
## cargo alone. Empty when the cargo has no model anywhere, which is not an error - plenty of
## loads are only mass.
static func load_model_filename(structure:MaszynaVehicleStructure, load_name:String) -> String:
    if not load_name:
        return ""
    var override:String = structure.load_models.get(load_name.to_lower(), "")
    if override:
        return override
    var specialized:String = "%s_%s" % [structure.file_name, load_name]
    if _model_exists(structure.data_path, specialized):
        return specialized
    var generic:String = load_name
    return generic if _model_exists(structure.data_path, generic) else ""


## The vehicle's exterior as its MMD names it - the body model, and its attachments as its
## children, drawn in its frame (DynObj.cpp:5384) - in the skin, drawn by RenderingServer instances
## (OPTIMIZED): a vehicle to look at - the vehicle viewer, a vehicle dragged into the editor's 3D
## view - not to run. Lies in the model's own frame; MASZYNA_VEHICLE_FRAME turns it as a vehicle.
static func build_exterior(data_path:String, file_name:String, skin:String, vehicle_name:String) -> E3DModelInstance:
    var normalized_data_path:String = data_path if data_path.begins_with("/") else "/" + data_path
    var game_dir:String = UserSettings.get_maszyna_game_dir()
    var relative_path:String = normalized_data_path.trim_prefix("/").path_join(file_name + ".mmd")
    var abs_mmd_path:String = game_dir.path_join(MaszynaDataPath.resolve(game_dir, relative_path))
    var parameters:Dictionary = MmdCabinInstancer.vehicle_parameters(vehicle_name, file_name, skin)
    var body_model:String = MmdCabinInstancer.parse_body_model(abs_mmd_path, parameters)
    if not body_model:
        body_model = file_name
    var skins:Array = MmdCabinInstancer.resolve_skins(normalized_data_path, skin)
    var exterior:E3DModelInstance = E3DModelInstance.new()
    exterior.instancer = E3DModelInstance.Instancer.OPTIMIZED
    exterior.data_path = normalized_data_path
    exterior.model_filename = body_model
    exterior.skins = skins
    for attachment_filename:String in MmdCabinInstancer.parse_attachments(abs_mmd_path, parameters):
        var attachment:E3DModelInstance = E3DModelInstance.new()
        attachment.instancer = E3DModelInstance.Instancer.OPTIMIZED
        attachment.data_path = normalized_data_path
        attachment.model_filename = attachment_filename
        attachment.skins = skins
        exterior.add_child(attachment)
    return exterior


static func _model_exists(data_path:String, relpath:String) -> bool:
    if not relpath:
        return false
    var game_dir:String = UserSettings.get_maszyna_game_dir()
    var relative_base_path:String = data_path.trim_prefix("/").path_join(relpath)
    var e3d_path:String = MaszynaDataPath.resolve(game_dir, relative_base_path + ".e3d")
    if FileAccess.file_exists(game_dir.path_join(e3d_path)):
        return true
    var t3d_path:String = MaszynaDataPath.resolve(game_dir, relative_base_path + ".t3d")
    return FileAccess.file_exists(game_dir.path_join(t3d_path))


## PackedScene.pack()-in-memory trick, already used in production by
## FizVehicleBuilder.build_scene() - CabinSystem.cabin_show() instantiates a correctly
## pre-configured MaszynaDynamicTrainCabin every time. The cab is drawn in the MaSzyna vehicle frame
## like the models (MASZYNA_VEHICLE_FRAME).
static func _build_cabin_scene(normalized_data_path:String, file_name:String, skin:String) -> PackedScene:
    var cabin := MaszynaDynamicTrainCabin.new()
    cabin.data_path = normalized_data_path
    cabin.mmd_filename = file_name
    cabin.skin = skin
    cabin.transform = MASZYNA_VEHICLE_FRAME
    var packed := PackedScene.new()
    var err:Error = packed.pack(cabin)
    cabin.free()
    if err != OK:
        push_error("MaszynaRailVehicle3DInstancer: could not pack cabin scene for %s/%s" % [normalized_data_path, file_name])
        return null
    return packed


## The submodels of the exterior model that move, by the names the MMD gives them; a name the
## model has not got is not animated, as in the original (DynObj.cpp:5351, 5414, 5848)
static func _resolve_parts(
        appearance:RailVehicleAppearance, model:E3DModel, wheels:RailVehicleWheels, wheel_names:PackedStringArray,
        pantograph_elements:Array[PackedStringArray], wiper_names:PackedStringArray,
        mirror_names:PackedStringArray, door_names:PackedStringArray, door_step_names:PackedStringArray,
        pendulums:Dictionary, pantograph_factors:PackedFloat64Array) -> void:
    var names:Dictionary[String, E3DSubModel] = {}
    _index_submodels(model.submodels, names)

    # the destination sign is the submodel with replaceable skin 4 (TSubModel::find_replacable4(),
    # Model3d.cpp:932-958; init_destination(), DynObj.cpp:2539-2545); none, no sign
    var sign:E3DSubModel = _find_destination_sign(model.submodels, 0)
    appearance.head_display_submodel = sign.resource_name if sign else ""

    appearance.front_bogie = _find_submodel(names, FRONT_BOGIE_SUBMODEL_NAMES)
    appearance.rear_bogie = _find_submodel(names, REAR_BOGIE_SUBMODEL_NAMES)

    # every axle turns as a powered one unless the rolling wheels have diameters of their own; then
    # the axle arrangement says which they are: letters powered, digits rolling - the front ones
    # before the first letter, the rear ones after it (DynObj.cpp:5361-5388)
    var groups:PackedInt32Array = []
    groups.resize(wheel_names.size())
    groups.fill(WheelGroup.POWERED)
    if wheels and (not wheels.front_rolling_wheel_diameter == wheels.powered_wheel_diameter
            or not wheels.rear_rolling_wheel_diameter == wheels.powered_wheel_diameter):
        var arrangement:String = wheels.axle_arrangement
        var axle:int = 0
        # the original reads the arrangement from its second character (j = 1, DynObj.cpp:5364)
        var next:int = 1
        var symbol:int = 0
        var rolling_group:WheelGroup = WheelGroup.FRONT_ROLLING
        while axle < groups.size() and next <= arrangement.length():
            if symbol >= POWERED_AXLES_FIRST and symbol <= POWERED_AXLES_LAST:
                groups[axle] = WheelGroup.POWERED
                axle += 1
                symbol -= 1
                rolling_group = WheelGroup.REAR_ROLLING
            elif symbol >= ROLLING_AXLES_FIRST and symbol <= ROLLING_AXLES_LAST:
                groups[axle] = rolling_group
                axle += 1
                symbol -= 1
            else:
                symbol = arrangement.unicode_at(next) if next < arrangement.length() else 0
                next += 1
    var wheel_groups:Array[PackedStringArray] = [PackedStringArray(), PackedStringArray(), PackedStringArray()]
    for index:int in range(wheel_names.size()):
        var wheel:String = _find_submodel(names, [wheel_names[index].to_lower()])
        if wheel:
            wheel_groups[groups[index]].append(wheel)
    appearance.front_rolling_wheels = wheel_groups[WheelGroup.FRONT_ROLLING]
    appearance.powered_wheels = wheel_groups[WheelGroup.POWERED]
    appearance.rear_rolling_wheels = wheel_groups[WheelGroup.REAR_ROLLING]

    var pantographs:Array[PackedStringArray] = []
    for pantograph:int in PANTOGRAPH_COUNT:
        pantographs.append(_find_pantograph_arms(names, pantograph_elements, pantograph))
    appearance.pantograph_factors = pantograph_factors
    appearance.pantograph_front_arms = pantographs[0]
    appearance.pantograph_rear_arms = pantographs[1]
    var wiper_arms:PackedStringArray = []
    for wiper_name:String in wiper_names:
        for element:String in WIPER_ELEMENT_SUFFIXES:
            wiper_arms.append(_find_submodel(names, [(wiper_name + element).to_lower()]))
    appearance.wiper_arms = wiper_arms
    var mirrors:PackedStringArray = []
    for mirror_name:String in mirror_names:
        mirrors.append(_find_submodel(names, [mirror_name.to_lower()]))
    appearance.mirrors = mirrors
    # a door with the first submodel below it and that one's, which a folding door turns as well
    # (UpdateDoorFold(), DynObj.cpp:592-622); "" for what the model has not got
    var doors:PackedStringArray = []
    for door_name:String in door_names:
        var door:E3DSubModel = names.get(door_name.to_lower())
        var below:E3DSubModel = _first_child(door)
        var further:E3DSubModel = _first_child(below)
        doors.append_array([door.resource_name.to_lower() if door else "", below.resource_name.to_lower() if below else "",
                further.resource_name.to_lower() if further else ""])
    appearance.doors = doors
    var steps:PackedStringArray = []
    for step_name:String in door_step_names:
        steps.append(_find_submodel(names, [step_name.to_lower()]))
    appearance.door_steps = steps
    var swinging:PackedStringArray = []
    for pendulum_name:String in pendulums["names"]:
        var pendulum:String = _find_submodel(names, [pendulum_name.to_lower()])
        if pendulum:
            swinging.append(pendulum)
    appearance.pendulums = swinging
    appearance.pendulum_amplitude = pendulums["amplitude"]


## The first submodel below `submodel` (TSubModel::ChildGet()), null without one
static func _first_child(submodel:E3DSubModel) -> E3DSubModel:
    if not submodel:
        return null
    for child:Variant in submodel.submodels:
        if child is E3DSubModel:
            return child
    return null


## The 5 element names of a pantograph (0 the front one), or none without its slider. An element the
## model has not got is an empty name and is not animated, as in the original (DynObj.cpp:644-661) -
## a single-arm pantograph has no second arms (dynamic/pkp/e186_v2 has no "ramiegorne2"), and one with
## no lower arm takes its geometry from the type and pantfactors: (RailVehicleRenderingServer).
static func _find_pantograph_arms(
        names:Dictionary[String, E3DSubModel], pantograph_elements:Array[PackedStringArray], pantograph:int) -> PackedStringArray:
    var arms:PackedStringArray = []
    for index:int in pantograph_elements.size():
        var element_names:PackedStringArray = pantograph_elements[index]
        var arm:String = _find_submodel(names, [element_names[pantograph].to_lower()]) \
                if pantograph < element_names.size() else ""
        if not arm and index in PANTOGRAPH_REQUIRED_ARMS:
            return PackedStringArray()
        arms.append(arm)
    return arms


## RailVehicleWipers has to know how many wipers the model has: from cab 2 they are numbered from the
## other end (DynObj.cpp:4062).
static func apply_wiper_count(vehicle:RID, appearance:RailVehicleAppearance) -> void:
    var wipers:RailVehicleWipers = VehicleServer.vehicle_component_get(
            vehicle, VehicleComponentType.COMPONENT_WIPERS) as RailVehicleWipers
    if wipers:
        wipers.wiper_count = appearance.wiper_arms.size() / WIPER_ELEMENT_SUFFIXES.size()
        wipers.apply_config()


static func _find_submodel(names:Dictionary[String, E3DSubModel], candidates:Array[String]) -> String:
    for candidate:String in candidates:
        if names.has(candidate):
            return candidate
    return ""


## Indexed by LOWERCASED name, matching the original engine's own TSubModel::GetFromName
## (case-insensitive by default) - see mmd_cabin_instancer.gd's _index_submodels for the
## real-data case mismatch (su45_v2) this guards against.
## The first submodel with replaceable skin 4 in the original's order: the submodel, then its next
## siblings and all below them, then its children (Model3d.cpp:932-958)
static func _find_destination_sign(submodels:Array, index:int) -> E3DSubModel:
    if index >= submodels.size():
        return null
    var submodel:E3DSubModel = submodels[index] as E3DSubModel
    if submodel and submodel.dynamic_material and submodel.dynamic_material_index == DESTINATION_SIGN_SKIN:
        return submodel
    var later:E3DSubModel = _find_destination_sign(submodels, index + 1)
    if later:
        return later
    return _find_destination_sign(submodel.submodels, 0) if submodel else null


static func _index_submodels(submodels:Array, names:Dictionary[String, E3DSubModel]) -> void:
    for item:Variant in submodels:
        var submodel:E3DSubModel = item as E3DSubModel
        if submodel:
            names[submodel.resource_name.to_lower()] = submodel
            _index_submodels(submodel.submodels, names)
