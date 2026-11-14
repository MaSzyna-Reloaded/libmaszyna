@tool
extends RefCounted
class_name MaszynaRailVehicle3DInstancer

## Reads what a vehicle's MMD says it is built from, out of nothing but a
## data_path/file_name/skin triple: its appearance (the models and the submodels that move, drawn
## by RailVehicleRenderingServer), the models of its cargo and its interactive MMD-driven cabin
## (CabinSystem). Used by MaszynaVehicleSystem, which builds the vehicle from it.

## The bogies, by the names the original itself looks for (DynObj.cpp:2482-2487). Everything else
## that moves - wheels, pantographs, wipers, mirrors - is named by the vehicle's MMD
## (MmdCabinInstancer.parse_*_names).
const FRONT_BOGIE_SUBMODEL_NAMES:Array[String] = ["bogie1", "boogie01"]
const REAR_BOGIE_SUBMODEL_NAMES:Array[String] = ["bogie2", "boogie02"]
## The elements of a wiper (DynObj.cpp:5845-5866)
const WIPER_ELEMENT_SUFFIXES:Array[String] = ["_p1", "_p2", "_p3"]
## How far wipers driven by hand turn [deg] - the data of no WiperList gives no angle
const MANUAL_WIPER_ANGLE:float = 90.0

## The pantograph element without which the pantograph is not animated: the slider (DynObj.cpp:5562-5566;
## an index of MmdCabinInstancer.PANTOGRAPH_ELEMENT_LABELS)
const PANTOGRAPH_REQUIRED_ARMS:Array[int] = [4]
## The front and the rear pantograph (Pantographs[end::front/rear], MOVER.h)
## An axle's diameter: the front rolling, the powered and the rear rolling one (dWheelAngle[0..2],
## DynObj.cpp:5362-5385)
## The destination sign's replaceable skin: material -4, held as its index 3 (MaszynaE3DModelBuilder.cpp:153)
const DESTINATION_SIGN_SKIN:int = 3
enum WheelGroup { FRONT_ROLLING, POWERED, REAR_ROLLING }
## The axle arrangement's symbols of powered axles, A to J, and of rolling ones, 1 to 9
## (DynObj.cpp:5372, 5378)
const POWERED_AXLES_FIRST:int = 65
const POWERED_AXLES_LAST:int = 74
const ROLLING_AXLES_FIRST:int = 49
const ROLLING_AXLES_LAST:int = 57

## Raised whenever read_mmd_data() changes what it puts into the data - the files' mtimes the
## cache of it is kept by (MaszynaVehicleMmdManager) cannot see it. 38: wiper arms of a model's patch; 37: the destination sign; 36: the model's description alone,
## the slider's height from its vertices; 35: the pantographs' geometry; 34: RoadVehicleAppearance of a road vehicle; 33: MaszynaRailVehicleAppearance;
## 32: the cabins the MMD defines; 31: coupleradapter:; 30: pantfactors:, a pantograph needs only its
## slider; 29: pendulums; 28: doors and door steps; 27: rolling wheels by the axle arrangement
const MMD_DATA_FORMAT_VERSION:int = 38

## Every MaSzyna-authored piece of a vehicle (exterior, low-poly interior, passengers, cab) lives in
## one vehicle-local frame where +Z is the direction of travel: the original draws all of them under
## the same TDynamicObject::mMatrix, built by BasisChange(vLeft, vUp, vFront) (DynObj.cpp:2506-2508;
## opengl33renderer.cpp:1174, 2856, 2976). A vehicle faces -Z here, so each of them is turned about
## its vertical by half a turn.
const MASZYNA_VEHICLE_FRAME:Transform3D = Transform3D(Basis(Vector3.UP, PI), Vector3.ZERO)
## A submodel's light threshold of a submodel never lit (`SelfIllum: false`, Model3d.cpp:343-351)
const NEVER_LIT:float = -1.0


## Reads what the vehicle's MMD says it is built from. This is the expensive half - every call
## opens and re-parses the MMD and loads the exterior model - and its result is what
## MaszynaVehicleSystem caches.
static func read_mmd_data(
        data_path:String, file_name:String, skin:String, vehicle_id:String) -> MaszynaVehicleMmdData:
    if not data_path or not file_name:
        return null

    # MaszynaMaterialManager.get_submodel_material() builds its material-search path by dropping
    # data_path's FIRST "/"-separated segment - meant to strip the artifact empty segment from a
    # LEADING slash, not the "dynamic" directory name itself. Every hand-authored vehicle scene
    # except su45 (whose skin is consequently broken the same way) uses a leading slash
    # (e.g. "/dynamic/pkp/ep09_v1/") for exactly this reason. Normalize here so operators don't
    # need to know about this quirk.
    var normalized_data_path:String = data_path if data_path.begins_with("/") else "/" + data_path
    var game_dir:String = MaszynaGameDirectory.get_game_dir()
    var relative_mmd_path:String = normalized_data_path.trim_prefix("/").path_join(file_name + ".mmd")
    var abs_mmd_path:String = game_dir.path_join(MaszynaDataPath.resolve(game_dir, relative_mmd_path))
    var parameters:Dictionary = MmdCabinInstancer.vehicle_parameters(vehicle_id, file_name, skin)

    var mmd_data := MaszynaVehicleMmdData.new()
    mmd_data.data_path = normalized_data_path
    mmd_data.file_name = file_name
    # a road vehicle is drawn with its wheels turned, nothing of a rail vehicle's; the wheels'
    # diameters and axle arrangement of a rail one split its axles into powered and rolling ones
    var description:VehicleController = MaszynaFizManager.load_description(
            normalized_data_path.trim_prefix("/"), file_name)
    var road_appearance:RoadVehicleAppearance = (
            RoadVehicleAppearance.new() if description is RoadVehicleController else null)
    var rail_appearance:MaszynaRailVehicleAppearance = (
            null if road_appearance else MaszynaRailVehicleAppearance.new())
    var appearance:VehicleAppearance = road_appearance if road_appearance else rail_appearance
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

    mmd_data.load_models = MmdCabinInstancer.parse_loads(abs_mmd_path, parameters)
    var passengers_filename:String = mmd_data.load_models.get("passengers", "")
    if passengers_filename:
        appearance.passengers_model_filename = passengers_filename

    appearance.skins = PackedStringArray(MmdCabinInstancer.resolve_skins(normalized_data_path, skin))
    # the model's description alone - none of its geometry, nothing in the GPU while the vehicle's
    # data is read
    var model:MaszynaE3DModel = MaszynaE3DModelManager.load_model_description(normalized_data_path, appearance.model_filename)
    if model:
        # the destination sign is the submodel with replaceable skin 4 (TSubModel::find_replacable4(),
        # Model3d.cpp:932-958; init_destination(), DynObj.cpp:2539-2545); none, no sign. It shines
        # unless it is never lit (has_light, Model3d.cpp:937)
        var sign:MaszynaE3DSubModel = _find_destination_sign(model.submodels, 0)
        if sign:
            mmd_data.destination_sign = MmdCabinInstancer.parse_destination_sign(
                    abs_mmd_path, parameters, normalized_data_path)
            mmd_data.destination_sign["submodel"] = sign.resource_name
            mmd_data.destination_sign["lit"] = not sign.lights_on_threshold == NEVER_LIT
    if model and road_appearance:
        var names:Dictionary[String, MaszynaE3DSubModel] = {}
        _index_submodels(model.submodels, names)
        var wheels:PackedStringArray = []
        for wheel_name:String in MmdCabinInstancer.parse_wheel_names(abs_mmd_path, parameters):
            var wheel:String = _find_submodel(names, [wheel_name.to_lower()])
            if wheel:
                wheels.append(wheel)
        road_appearance.wheels = wheels
    if rail_appearance:
        rail_appearance.joint_cabs = MmdCabinInstancer.parse_joint_cabs(abs_mmd_path, parameters)
    if model and rail_appearance:
        var wheels:RailVehicleWheels = null
        if description:
            for component:VehicleComponent in description.get_components():
                if component is RailVehicleWheels:
                    wheels = component
        _resolve_parts(rail_appearance, model, wheels, MmdCabinInstancer.parse_wheel_names(abs_mmd_path, parameters),
                MmdCabinInstancer.parse_pantograph_element_names(abs_mmd_path, parameters),
                MmdCabinInstancer.parse_wiper_names(abs_mmd_path, parameters),
                MmdCabinInstancer.parse_mirror_names(abs_mmd_path, parameters),
                MmdCabinInstancer.parse_door_names(abs_mmd_path, parameters),
                MmdCabinInstancer.parse_door_step_names(abs_mmd_path, parameters),
                MmdCabinInstancer.parse_pendulums(abs_mmd_path, parameters),
                MaszynaVehicleSystem.model_get_patch(normalized_data_path.path_join(appearance.model_filename)))
        mmd_data.pantograph_geometries = _measure_pantographs(
                rail_appearance, model,
                MaszynaE3DModelManager.get_model_path(normalized_data_path, appearance.model_filename), description,
                MmdCabinInstancer.parse_pantograph_factors(abs_mmd_path, parameters))
    mmd_data.appearance = appearance
    mmd_data.coupler_adapter = MmdCabinInstancer.parse_coupler_adapter(abs_mmd_path, parameters)
    mmd_data.cabin_scene = _build_cabin_scene(normalized_data_path, file_name, skin)
    mmd_data.cabin_kinds = MmdCabinInstancer.parse_cabin_kinds(abs_mmd_path, parameters)
    return mmd_data


## Which model a cargo is drawn as, in the order the original tries them
## (TDynamicObject::LoadMMediaFile_mdload(), DynObj.cpp:7195): the vehicle's own override for that
## cargo, then a model named for this vehicle and the cargo together, then one named after the
## cargo alone. Empty when the cargo has no model anywhere, which is not an error - plenty of
## loads are only mass - and for a cargo the vehicle does not accept: the original assigns none of
## those (TMoverParameters::AssignLoad(), Mover.cpp: "this type is unsupported") and draws the model
## of the load it took (DynObj.cpp:3008).
static func load_model_filename(mmd_data:MaszynaVehicleMmdData, load_name:String,
        accepted_loads:Array[String]) -> String:
    if not load_name or not accepted_loads.has(load_name):
        return ""
    var override:String = mmd_data.load_models.get(load_name.to_lower(), "")
    if override:
        return override
    var specialized:String = "%s_%s" % [mmd_data.file_name, load_name]
    if _model_exists(mmd_data.data_path, specialized):
        return specialized
    var generic:String = load_name
    return generic if _model_exists(mmd_data.data_path, generic) else ""


## The vehicle's exterior as its MMD names it - the body model, and its attachments as its
## children, drawn in its frame (DynObj.cpp:5384) - in the skin, drawn by RenderingServer instances
## (OPTIMIZED): a vehicle to look at - the vehicle viewer, a vehicle dragged into the editor's 3D
## view - not to run. Lies in the model's own frame; MASZYNA_VEHICLE_FRAME turns it as a vehicle.
static func build_exterior(data_path:String, file_name:String, skin:String, vehicle_id:String) -> MaszynaE3DModelInstance:
    var normalized_data_path:String = data_path if data_path.begins_with("/") else "/" + data_path
    var game_dir:String = MaszynaGameDirectory.get_game_dir()
    var relative_path:String = normalized_data_path.trim_prefix("/").path_join(file_name + ".mmd")
    var abs_mmd_path:String = game_dir.path_join(MaszynaDataPath.resolve(game_dir, relative_path))
    var parameters:Dictionary = MmdCabinInstancer.vehicle_parameters(vehicle_id, file_name, skin)
    var body_model:String = MmdCabinInstancer.parse_body_model(abs_mmd_path, parameters)
    if not body_model:
        body_model = file_name
    var skins:Array = MmdCabinInstancer.resolve_skins(normalized_data_path, skin)
    var exterior:MaszynaE3DModelInstance = MaszynaE3DModelInstance.new()
    exterior.instancer = MaszynaE3DModelInstance.Instancer.OPTIMIZED
    exterior.data_path = normalized_data_path
    exterior.model_filename = body_model
    exterior.skins = skins
    for attachment_filename:String in MmdCabinInstancer.parse_attachments(abs_mmd_path, parameters):
        var attachment:MaszynaE3DModelInstance = MaszynaE3DModelInstance.new()
        attachment.instancer = MaszynaE3DModelInstance.Instancer.OPTIMIZED
        attachment.data_path = normalized_data_path
        attachment.model_filename = attachment_filename
        attachment.skins = skins
        exterior.add_child(attachment)
    return exterior


static func _model_exists(data_path:String, relpath:String) -> bool:
    if not relpath:
        return false
    var game_dir:String = MaszynaGameDirectory.get_game_dir()
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
    var packed:PackedScene = ResourceCache.scene_pack(cabin)
    cabin.free()
    if not packed:
        push_error("MaszynaRailVehicle3DInstancer: could not pack cabin scene for %s/%s" % [normalized_data_path, file_name])
        return null
    return packed


## The submodels of the exterior model that move, by the names the MMD gives them; a name the
## model has not got is not animated, as in the original (DynObj.cpp:5351, 5414, 5848)
static func _resolve_parts(
        appearance:MaszynaRailVehicleAppearance, model:MaszynaE3DModel, wheels:RailVehicleWheels, wheel_names:PackedStringArray,
        pantograph_elements:Array[PackedStringArray], wiper_names:PackedStringArray,
        mirror_names:PackedStringArray, door_names:PackedStringArray, door_step_names:PackedStringArray,
        pendulums:Dictionary, patch:MaszynaVehicleModelPatch) -> void:
    var names:Dictionary[String, MaszynaE3DSubModel] = {}
    _index_submodels(model.submodels, names)

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

    # every pantograph the MMD declares - as many as its arms' names, the slider's
    var pantographs:Array[PackedStringArray] = []
    for pantograph:int in pantograph_elements[PANTOGRAPH_REQUIRED_ARMS[0]].size():
        pantographs.append(_find_pantograph_arms(names, pantograph_elements, pantograph))
    appearance.pantograph_arms = pantographs
    var wiper_arms:PackedStringArray = []
    for wiper_name:String in wiper_names:
        for element:String in WIPER_ELEMENT_SUFFIXES:
            wiper_arms.append(_find_submodel(names, [(wiper_name + element).to_lower()]))
    # Quirk: the wiper arms of a model whose MMD names none, from the game's patch of the model -
    # each one arm, turned whole about its own point and axis by its own angle; one the model has
    # not got is not animated, its wiper kept in its place
    if not wiper_names and patch:
        var pivots:PackedVector3Array = []
        var axes:PackedVector3Array = []
        var turns:PackedFloat64Array = []
        for arm:MaszynaWiperArmPatch in patch.elements_of(MaszynaWiperArmPatch):
            var submodel:MaszynaE3DSubModel = MaszynaVehicleModelPatch.find_submodel(model.submodels, arm.submodel)
            if not submodel:
                push_warning("MaszynaRailVehicle3DInstancer: %s has no wiper arm %s" % [patch.path, arm.submodel])
            wiper_arms.append_array([submodel.resource_name if submodel else "", "", ""])
            pivots.append(arm.pivot)
            axes.append(arm.axis)
            turns.append(arm.turn)
        appearance.wiper_pivots = pivots
        appearance.wiper_axes = axes
        appearance.wiper_turns = turns
    appearance.wiper_arms = wiper_arms
    var mirrors:PackedStringArray = []
    for mirror_name:String in mirror_names:
        mirrors.append(_find_submodel(names, [mirror_name.to_lower()]))
    appearance.mirrors = mirrors
    # a door with the first submodel below it and that one's, which a folding door turns as well
    # (UpdateDoorFold(), DynObj.cpp:592-622); "" for what the model has not got
    var doors:PackedStringArray = []
    for door_name:String in door_names:
        var door:MaszynaE3DSubModel = names.get(door_name.to_lower())
        var below:MaszynaE3DSubModel = _first_child(door)
        var further:MaszynaE3DSubModel = _first_child(below)
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
static func _first_child(submodel:MaszynaE3DSubModel) -> MaszynaE3DSubModel:
    if not submodel:
        return null
    for child:Variant in submodel.submodels:
        if child is MaszynaE3DSubModel:
            return child
    return null


## The 5 element names of a pantograph (0 the front one), or none without its slider. An element the
## model has not got is an empty name and is not animated, as in the original (DynObj.cpp:644-661) -
## a single-arm pantograph has no second arms (dynamic/pkp/e186_v2 has no "ramiegorne2"), and one with
## no lower arm takes its geometry from the type and pantfactors: (RailVehicleRenderingServer).
static func _find_pantograph_arms(
        names:Dictionary[String, MaszynaE3DSubModel], pantograph_elements:Array[PackedStringArray], pantograph:int) -> PackedStringArray:
    var arms:PackedStringArray = []
    for index:int in pantograph_elements.size():
        var element_names:PackedStringArray = pantograph_elements[index]
        var arm:String = _find_submodel(names, [element_names[pantograph].to_lower()]) \
                if pantograph < element_names.size() else ""
        if not arm and index in PANTOGRAPH_REQUIRED_ARMS:
            return PackedStringArray()
        arms.append(arm)
    return arms


## The geometry of the FIZ's pantographs it does not give itself, measured from the model's data -
## pantograph i by the MMD's pantograph i - as MaszynaPantographGeometryReader reads it; the FIZ's
## comes first, so one it gives is not measured. Empty for a vehicle the FIZ names no pantograph of.
static func _measure_pantographs(
        appearance:MaszynaRailVehicleAppearance, model:MaszynaE3DModel, model_path:String,
        description:VehicleController, factors:PackedFloat64Array) -> Array[RailVehiclePantograph]:
    var measured:Array[RailVehiclePantograph] = []
    var pantographs:RailVehiclePantographs = null
    var power_source:RailVehicleEnginePowerSource = null
    for component:VehicleComponent in description.get_components() if description else []:
        if component is RailVehiclePantographs:
            pantographs = component
        elif component is RailVehicleEnginePowerSource:
            power_source = component
    if not pantographs or not power_source:
        return measured
    var arms:Array[PackedStringArray] = []
    arms.assign(appearance.pantograph_arms)
    for index:int in pantographs.pantographs.size():
        var given:RailVehiclePantograph = pantographs.pantographs[index]
        if given and given.has_geometry:
            measured.append(given)
            continue
        measured.append(MaszynaPantographGeometryReader.read(
                model, model_path, arms[index] if index < arms.size() else PackedStringArray(), appearance.model_transform,
                power_source.current_collector_pantograph_type, factors, index, description.dimensions_height,
                given.control if given else RailVehicleEnginePowerSource.PANTOGRAPH_FIRST))
    return measured


## RailVehicleWipers has to know how many wipers the model has: from cab 2 they are numbered from the
## other end (DynObj.cpp:4062).
##
## Quirk: a FIZ of no WiperList gives the wipers no switch - the original's do not move at all, even
## those of a model that animates them, and an old cab's are moved by hand (the EU07's handles,
## MaszynaCabinManualWipers). Such wipers are taken as driven by hand: as many as the model
## animates (its animwiperprefix:, or the wiper arms the game's patch gives it), or
## every one the cab's glass can wipe (RailVehicleWipers.MANUAL_WIPERS_MAX, the channels of its
## wiper mask) - the driver wipes the glass by the keys even of a model of none.
static func apply_wiper_count(vehicle:RID, appearance:MaszynaRailVehicleAppearance) -> void:
    var wipers:RailVehicleWipers = VehicleServer.vehicle_component_get(
            vehicle, VehicleComponentType.COMPONENT_WIPERS) as RailVehicleWipers
    if not wipers:
        return
    var count:int = appearance.wiper_arms.size() / WIPER_ELEMENT_SUFFIXES.size()
    if wipers.positions.is_empty():
        # a FIZ of no WiperList has no wiper angle either
        if is_zero_approx(wipers.angle):
            wipers.angle = MANUAL_WIPER_ANGLE
        wipers.manual_fit(count if count > 0 else RailVehicleWipers.MANUAL_WIPERS_MAX)
        return
    # a model of no animated arms leaves the count its patch gives (MaszynaWiperListPatch.wiper_count)
    if count > 0:
        wipers.wiper_count = count
    wipers.apply_config()


static func _find_submodel(names:Dictionary[String, MaszynaE3DSubModel], candidates:Array[String]) -> String:
    for candidate:String in candidates:
        if names.has(candidate):
            return candidate
    return ""


## Indexed by LOWERCASED name, matching the original engine's own TSubModel::GetFromName
## (case-insensitive by default) - see mmd_cabin_instancer.gd's _index_submodels for the
## real-data case mismatch (su45_v2) this guards against.
## The first submodel with replaceable skin 4 in the original's order: the submodel, then its next
## siblings and all below them, then its children (Model3d.cpp:932-958)
static func _find_destination_sign(submodels:Array, index:int) -> MaszynaE3DSubModel:
    if index >= submodels.size():
        return null
    var submodel:MaszynaE3DSubModel = submodels[index] as MaszynaE3DSubModel
    if submodel and submodel.dynamic_material and submodel.dynamic_material_index == DESTINATION_SIGN_SKIN:
        return submodel
    var later:MaszynaE3DSubModel = _find_destination_sign(submodels, index + 1)
    if later:
        return later
    return _find_destination_sign(submodel.submodels, 0) if submodel else null


static func _index_submodels(submodels:Array, names:Dictionary[String, MaszynaE3DSubModel]) -> void:
    for item:Variant in submodels:
        var submodel:MaszynaE3DSubModel = item as MaszynaE3DSubModel
        if submodel:
            names[submodel.resource_name.to_lower()] = submodel
            _index_submodels(submodel.submodels, names)
