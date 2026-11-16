extends Cabin3D
class_name MaszynaDynamicTrainCabin

## MMD-driven cabin builder, analogous to MaszynaE3DModelInstance/MaszynaRailVehiclePhysicsNode: given
## data_path/mmd_filename/skin it parses the vehicle's MMD file, reads the cab definition of the kind
## of its cabin (Cabin3D.cabin - cab1 the front cab, cab2 the rear one, cab0 the machine room), and
## builds a real, interactive cabin (Etap A+B scope - see mmd_cabin_instancer.gd) instead of
## requiring a hand-authored cabin_scene.
##
## Deliberately overrides _ready() and does not call super(): the base Cabin3D._ready() emits
## cabin_ready immediately, before this class's own children (cab model, widgets) exist -
## readiness here must wait until the whole MMD-derived "Generated" subtree is actually built.
## Everything below runs synchronously within one _ready() call (MMD parsing and E3D loading
## are both synchronous), so CabinSystem.cabin_show()'s wait on cabin_ready still resolves
## within the same add_child() call that creates this node.
##
## A view of the vehicle's cab only: the cab logic it shows is the vehicle's
## (MaszynaVehicleSystem attaches it, CabinSystem registers it for the driver's cabin), and the player's keys reach it
## without this node (Player).

@export var data_path:String = ""
@export var mmd_filename:String = ""
@export var skin:String = ""

## Cab light height above the driver's eyes, kept under the top of the cab model - the model top
## is its outer shell (roof pipes, heaters), not the ceiling the light has to stay under.
const CAB_LIGHT_ABOVE_DRIVER:float = 0.4
const CAB_LIGHT_CEILING_OFFSET:float = 0.15
## Quirk: MMD has no cab light position (cablight: holds only unused colors, Train.cpp:118-133), so the
## light goes to the cab model's ceiling lamp, found by the submodel names the game's cab models use
## for it (most specific first); CAB_LIGHT_ABOVE_DRIVER is the fallback without such a lamp.
const CAB_LAMP_SUBMODEL_NAMES:Array[String] = [
    "lampa_suf0", "lampy_sufit", "lampa_sufi", "lampa_sufit", "lampasufitowa", "lampasufit", "swiatlo_sufit",
    "cablight", "lampa",
]
## Distance of the cab light below the found ceiling lamp - inside the lamp's shadow casting mesh
## it would light nothing.
const CAB_LIGHT_BELOW_LAMP:float = 0.05
## The fields of a cab widget that name the submodels it moves or lights
const MOVED_PATH_FIELDS:Array[StringName] = [&"mesh_path", &"target_mesh_path", &"on_target_path", &"off_target_path"]

var _generated:Node3D
var _diagnostics:Array[Dictionary] = []
var _random_choices:Dictionary = {}
## The cab model's meshes as CabinHUDMouseSystem occluders - the desk hides what runs under it
var _occluders:Array[RID] = []
## What the cab is built of, if whoever shows it gives it (set_definition_reader()), else its file
var _definition_reader:Callable = Callable()
## The definition the cab is built of, and the nodes each of its controls is built as
var _definition:MmdCabinDefinition = null
var _instrument_nodes:Dictionary[MmdInstrumentDescriptor, Array] = {}
## The pose each node of the cab's model is built in, before any control moves it
var _rest_transforms:Dictionary[Node3D, Transform3D] = {}


## A cab of an MMD file is built by MaSzyna's cab implementation
func _init() -> void:
    implementation = MaszynaCabinImplementation.NAME


func _ready() -> void:
    vehicle_rid_changed.connect(_on_vehicle_rid_changed)
    cabin_changed.connect(_on_cabin_changed)
    # the MMD and the models are the game directory's
    GameDataServer.data_reload_requested.connect(reload)
    ProjectSettings.settings_changed.connect(_apply_reverse_cull_face)
    # Cabin3D's own _ready() emits cabin_ready; the engine calls it beside this one.


## Cabin3D announces the vehicle of its cabin rather than letting a subclass override set_cabin():
## CabinSystem calls that method typed, so a script method of the same name would never run.
func _on_vehicle_rid_changed(_vehicle_rid:RID) -> void:
    _rebuild_generated()


## Rebuilt for another cabin of the vehicle (cab0 = machine room, cab1, cab2)
func _on_cabin_changed(_cabin:RID) -> void:
    _rebuild_generated()


func _exit_tree() -> void:
    # the announcement goes first: clearing the cabin would otherwise rebuild the cab on its way
    # out of the tree
    vehicle_rid_changed.disconnect(_on_vehicle_rid_changed)
    cabin_changed.disconnect(_on_cabin_changed)
    GameDataServer.data_reload_requested.disconnect(reload)
    ProjectSettings.settings_changed.disconnect(_apply_reverse_cull_face)
    set_cabin(RID())
    _free_occluders()


func get_diagnostics() -> Array[Dictionary]:
    return _diagnostics


func reload() -> void:
    _rebuild_generated()


## Whoever shows the cab may hand it the definitions it is built of, in place of its MMD file's -
## an editor's cab as edited: reader(cab_number:int) -> MmdCabinDefinition (cab0definition: 0). An
## invalid callable reads the file again. Takes effect at the next build (reload())
func set_definition_reader(reader:Callable) -> void:
    _definition_reader = reader


## A control of the definition the cab is built of, built into the cab as it stands - an editor's
## control added: the rest of the cab, and where its camera looks, as they are
func instrument_add(descriptor:MmdInstrumentDescriptor) -> void:
    var model:Node3D = _generated.get_node_or_null("CabModel") if _generated else null
    var sound_player:SfxPlayer3D = _generated.get_node_or_null("CabinControlsSfxPlayer3D") if _generated else null
    if not model or not sound_player or _instrument_nodes.has(descriptor):
        return
    var sound_events:Array[SfxEvent] = []
    sound_events.assign(sound_player.bank.events)
    var diagnostics:Array[Dictionary] = []
    _instrument_nodes[descriptor] = MmdCabinInstancer.instrument_build_into(
            _generated, descriptor, get_vehicle_rid(), _definition.cab_number, _definition.driver_pos,
            MmdCabinInstancer.submodel_index(model), sound_player, sound_events,
            MaszynaGameDirectory.data_path_resolve(data_path.trim_prefix("/")), diagnostics)
    sound_player.bank.events = sound_events
    _diagnostics.append_array(diagnostics)


## A control taken out of the cab as it stands - an editor's control removed: the submodels it
## moved back in the pose the model builds them in, so that a control built on them anew starts there
func instrument_remove(descriptor:MmdInstrumentDescriptor) -> void:
    for node:Node in _instrument_nodes.get(descriptor, []):
        if not is_instance_valid(node):
            continue
        for field:StringName in MOVED_PATH_FIELDS:
            var path:Variant = node.get(field)
            if path is NodePath and not (path as NodePath).is_empty():
                var moved:Node3D = node.get_node_or_null(path) as Node3D
                if moved:
                    _rest_transforms_restore(moved)
        node.get_parent().remove_child(node)
        node.queue_free()
    _instrument_nodes.erase(descriptor)


func _rest_transforms_keep(node:Node) -> void:
    var spatial:Node3D = node as Node3D
    if spatial:
        _rest_transforms[spatial] = spatial.transform
    for child:Node in node.get_children(true):
        _rest_transforms_keep(child)


func _rest_transforms_restore(node:Node) -> void:
    var spatial:Node3D = node as Node3D
    if spatial and _rest_transforms.has(spatial):
        spatial.transform = _rest_transforms[spatial]
    for child:Node in node.get_children(true):
        _rest_transforms_restore(child)


## The submodels the widgets of a control move or light, as the cab wired them - what the mouse
## outlines of it; none for a control the cab has not built
func instrument_submodels(descriptor:MmdInstrumentDescriptor) -> Array[Node3D]:
    var submodels:Array[Node3D] = []
    for node:Node in _instrument_nodes.get(descriptor, []):
        if not is_instance_valid(node) or not node.is_inside_tree():
            continue
        for field:StringName in MOVED_PATH_FIELDS:
            var path:Variant = node.get(field)
            if path is NodePath and not (path as NodePath).is_empty():
                var moved:Node3D = node.get_node_or_null(path) as Node3D
                if moved and not moved in submodels:
                    submodels.append(moved)
    return submodels


## A control built anew in the cab as it stands - an editor's control moved to another submodel or
## animated otherwise
func instrument_rebuild(descriptor:MmdInstrumentDescriptor) -> void:
    instrument_remove(descriptor)
    instrument_add(descriptor)


func _rebuild_generated() -> void:
    _free_occluders()
    if _generated:
        remove_child(_generated)
        _generated.queue_free()
        _generated = null

    _diagnostics.clear()
    if not mmd_filename or not get_vehicle_rid() or not get_cabin().is_valid():
        return

    var cab_definition:int = MmdCabinInstancer.cab_definition(RailVehicleServer.cabin_get_kind(get_cabin()))

    var abs_mmd_path:String = MaszynaGameDirectory.data_path_resolve(
            data_path.trim_prefix("/").path_join(mmd_filename + ".mmd"))
    var definition:MmdCabinDefinition = null
    if _definition_reader.is_valid():
        definition = _definition_reader.call(cab_definition)
    else:
        var parameters:Dictionary = MmdCabinInstancer.vehicle_parameters(
                VehicleServer.vehicle_get_id(get_vehicle_rid()), mmd_filename, skin)
        definition = MmdCabinInstancer.parse(abs_mmd_path, parameters, cab_definition, _random_choices)
    _diagnostics.append_array(definition.diagnostics)

    camera_bound_min = definition.bounds_min
    camera_bound_max = definition.bounds_max
    camera_bound_enabled = true
    has_cab_model = true if definition.model_relpath else false
    # the camera starts at the seat, turned as the MMD says (drivermode.cpp:1224-1226)
    driver_position = definition.driver_sitpos
    driver_view_angle = definition.driver_angle
    shake_spring_stiffness = definition.shake_spring_stiffness
    shake_spring_damping = definition.shake_spring_damping
    shake_jolt_scale = definition.shake_jolt_scale
    shake_jolt_limit = definition.shake_jolt_limit
    shake_angle_scale = definition.shake_angle_scale
    engine_shake_scale = definition.engine_shake_scale
    engine_shake_fade_in_rpm = definition.engine_shake_fade_in_rpm
    engine_shake_fade_in_factor = definition.engine_shake_fade_in_factor
    engine_shake_fade_out_rpm = definition.engine_shake_fade_out_rpm
    engine_shake_fade_out_factor = definition.engine_shake_fade_out_factor

    _generated = Node3D.new()
    _generated.name = "Generated"
    add_child(_generated, false, INTERNAL_MODE_BACK)

    var build_diagnostics:Array[Dictionary] = []
    _definition = definition
    _instrument_nodes.clear()
    _rest_transforms.clear()
    MmdCabinInstancer.build_into(
            _generated, definition, get_vehicle_rid(), data_path, skin, build_diagnostics, _instrument_nodes)
    var built_model:Node3D = _generated.get_node_or_null("CabModel")
    if built_model:
        _rest_transforms_keep(built_model)
    _diagnostics.append_array(build_diagnostics)

    var cab_model:MaszynaE3DModelInstance = _generated.get_node_or_null("CabModel") as MaszynaE3DModelInstance
    if cab_model:
        if cab_model.is_e3d_loaded():
            _create_occluders(cab_model)
        else:
            cab_model.e3d_loaded.connect(_create_occluders.bind(cab_model), CONNECT_ONE_SHOT)

    # a cab with an i-cablight lamp already has its own ceiling light (MmdSemanticCatalog)
    if not definition.instruments.any(
            func(instrument:MmdInstrumentDescriptor) -> bool: return instrument.label == "i-cablight"):
        _build_cab_light(definition)
    var windscreen_wipers := CabinWindscreenWipers.new()
    windscreen_wipers.name = "WindscreenWipers"
    windscreen_wipers.vehicle_rid = get_vehicle_rid()
    _generated.add_child(windscreen_wipers)
    camera_configuration_changed.emit()
    _apply_reverse_cull_face()

    print("MaszynaDynamicTrainCabin: built cab %d from %s - %d instruments parsed, %d generated children" % [
        cab_definition, abs_mmd_path, definition.instruments.size(), _generated.get_child_count()])
    for d:Dictionary in _diagnostics:
        print("  [%s] %s (label=%s submodel=%s)" % [d["severity"], d["message"], d["mmd_label"], d["submodel_name"]])


## Every light of the cab draws its shadow map with the faces the setting says
## (opengl33renderer.cpp:1758) - once the cab is built, and again when the setting changes
func _apply_reverse_cull_face() -> void:
    if not _generated:
        return
    var reverse:bool = ProjectSettings.get_setting(SkyEnvironment.REVERSE_CULL_FACE_SETTING)
    for light:Node in _generated.find_children("*", "Light3D", true, false):
        (light as Light3D).shadow_reverse_cull_face = reverse


## Every opaque mesh of the cab model hides the controls behind it from the mouse, as the cab is
## drawn into the original's pick buffer without its translucent submodels
## (opengl33renderer.cpp:1208) - E186's spring brake buttons lie under glass caps. Control meshes
## are among them and keep their own hits.
func _create_occluders(cab_model:MaszynaE3DModelInstance) -> void:
    for mesh_id:int in MaszynaE3DRenderingServer.instance_get_opaque_meshes(cab_model.get_model_instance()):
        _occluders.append(CabinHUDMouseSystem.occluder_create(mesh_id))


func _free_occluders() -> void:
    for occluder:RID in _occluders:
        CabinHUDMouseSystem.occluder_free(occluder)
    _occluders.clear()


## Cab interior lighting: the original lights the cab model with a tungsten ambient term
## InteriorLight * InteriorLightLevel (DynObj.h:240, openglrenderer.cpp:3684-3692); here a shadow
## casting light at the cab ceiling lamp, driven by the same level - the cab light of this cab
## (CabinSystem.cabin_light_level_changed, Train.cpp:8436-8453).
func _build_cab_light(definition:MmdCabinDefinition) -> void:
    var light := CabinOmniLight3D.new()
    light.name = "CabLight"
    light.light_color = definition.interior_light
    light.shadow_enabled = true
    # without a cab model: the top of the camera bounds
    light.position = (definition.bounds_min + definition.bounds_max) * 0.5
    light.position.y = definition.bounds_max.y
    light.omni_range = maxf((definition.bounds_max - definition.bounds_min).length(), 1.0)
    light.cab_light = CabinState.Light.CAB
    _generated.add_child(light)
    light.set_vehicle_rid(get_vehicle_rid())

    var cab_model:MaszynaE3DModelInstance = _generated.get_node_or_null("CabModel") as MaszynaE3DModelInstance
    if not cab_model:
        return
    if cab_model.is_e3d_loaded():
        _place_cab_light(light, cab_model, definition.driver_pos.y)
    else:
        cab_model.e3d_loaded.connect(
                _place_cab_light.bind(light, cab_model, definition.driver_pos.y), CONNECT_ONE_SHOT)


## Just under the cab model's ceiling lamp; without one, horizontal center of the cab model above
## the driver's eyes.
func _place_cab_light(light:CabinOmniLight3D, cab_model:MaszynaE3DModelInstance, driver_height:float) -> void:
    var bounds:AABB = cab_model.submodels_aabb
    light.omni_range = maxf(bounds.size.length(), 1.0)
    var lamp_bounds:AABB = _find_cab_lamp_bounds(cab_model)
    if lamp_bounds.has_volume() or lamp_bounds.has_surface():
        var lamp_center:Vector3 = lamp_bounds.get_center()
        light.position = cab_model.transform * Vector3(
                lamp_center.x, lamp_bounds.position.y - CAB_LIGHT_BELOW_LAMP, lamp_center.z)
        return
    var center:Vector3 = bounds.get_center()
    var height:float = minf(driver_height + CAB_LIGHT_ABOVE_DRIVER, bounds.end.y - CAB_LIGHT_CEILING_OFFSET)
    light.position = cab_model.transform * Vector3(center.x, height, center.z)


## Bounds (in cab model space) of the first ceiling lamp submodel found by CAB_LAMP_SUBMODEL_NAMES,
## matched case-insensitively like TSubModel::GetFromName, also as an _on/_off indicator pair.
func _find_cab_lamp_bounds(cab_model:MaszynaE3DModelInstance) -> AABB:
    var meshes_by_name:Dictionary = {}
    for mesh:Node in cab_model.find_children("*", "MeshInstance3D", true, false):
        var mesh_name:String = String(mesh.name).to_lower()
        if not meshes_by_name.has(mesh_name):
            meshes_by_name[mesh_name] = mesh
    for lamp_name:String in CAB_LAMP_SUBMODEL_NAMES:
        for suffix:String in ["", "_on", "_off"]:
            var mesh:MeshInstance3D = meshes_by_name.get(lamp_name + suffix) as MeshInstance3D
            if mesh:
                return cab_model.global_transform.affine_inverse() * mesh.global_transform * mesh.get_aabb()
    return AABB()
