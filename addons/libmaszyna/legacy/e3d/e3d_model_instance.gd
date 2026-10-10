@tool
extends VisualInstance3D
class_name E3DModelInstance

## Displays a MaSzyna E3D model in 3D space.
##
## [E3DModelInstance] is a node client of [E3DRenderingServer]: it loads an [E3DModel] and keeps
## one server instance built with the selected instancer. [code]NODES[/code] and
## [code]EDITABLE_NODES[/code] build a regular node hierarchy under this node.
## [code]OPTIMIZED[/code] renders through [code]RenderingServer[/code] and does not create child nodes.
## If [member model] is set, it will be used to instnatiate. Otherwise the
## [member data_path] and [member model_filename] will be used to load with [E3DModelManager].
## With [method set_e3d_instance] the node stands for an instance somebody else owns instead.


## Emitted after the current model instance has been created.
signal e3d_loading
signal e3d_loaded
## Emitted whenever a new [E3DRenderingServer] instance is created for the model - on the first
## load, on [method reload] and on re-entering the tree
signal e3d_instance_created(instance: RID)

## Selected instancing backend (same values as [enum E3DRenderingServer.Instancer])
enum Instancer {
     OPTIMIZED,  ## Renders using [code]RenderingServer[/code] without creating child mesh nodes.
     NODES,  ## Creates a regular hierarchy of generated 3D nodes.
     EDITABLE_NODES,  ## Creates generated 3D nodes intended to stay editable in the editor.
}

var _model: E3DModel
var _dirty: bool = false
var _e3d_loaded: bool = false
var _rid: RID = RID()
## _rid is somebody else's (set_e3d_instance()): attached and switched here, never created or freed
var _proxy: bool = false

@export var lights_state: Dictionary[String, bool] = {}:
    set(x):
        lights_state = _merge_lights_state(x)
        if _rid.is_valid():
            E3DRenderingServer.instance_set_lights_state(_rid, lights_state)

## Light name -> dimmed: the "_xon" submodel instead of "_on", and the real light at
## [member lights_dimmed_multiplier] (see [method E3DRenderingServer.instance_set_lights_dimmed])
@export var lights_dimmed: Dictionary[String, bool] = {}:
    set(x):
        lights_dimmed = x
        if _rid.is_valid():
            E3DRenderingServer.instance_set_lights_dimmed(_rid, lights_dimmed, lights_dimmed_multiplier)

@export var lights_dimmed_multiplier: float = 1.0:
    set(x):
        lights_dimmed_multiplier = x
        if _rid.is_valid():
            E3DRenderingServer.instance_set_lights_dimmed(_rid, lights_dimmed, lights_dimmed_multiplier)


var default_aabb_size: Vector3 = Vector3(1, 1, 1)

## E3DModel to instantiate (leave empty, if you want to lazy load with [member model_filename])
@export var model: E3DModel:
    set(x):
        if not x == model:
            model = x
            _dirty = true

## What this placement is - a static scenery model or a dynamic one, in the words a [code].scn[/code]
## uses. Handed to the server when the instance is created, because the smoke density it selects is
## baked into every emitter of the model, and a dynamic one hides its "_on" controls until they are
## switched (Model3d.cpp:2221); changing it reloads the instance, like the model itself.
@export var instance_kind: E3DRenderingServer.InstanceKind = E3DRenderingServer.INSTANCE_KIND_STATIC:
    set(x):
        if not x == instance_kind:
            instance_kind = x
            _dirty = true

## Base MaSzyna data path used to resolve model files and materials.
@export var data_path:String = "":
    set(x):
        if not x == data_path:
            data_path = x
            _dirty = true

@export var model_filename:String = "":
    set(x):
        if not x == model_filename:
            model_filename = x
            _dirty = true

@export var skins:Array = []:
    set(x):
        if not x == skins:
            skins = x
            _dirty = true

@export var exclude_node_names:Array = []:
    set(x):
        if not x == exclude_node_names:
            exclude_node_names = x
            _dirty = true

## Submodel tree paths (relative to the model root, e.g. [code]"banan/podswietlenie_on"[/code])
## for which real alpha blending is forced, instead of the default alpha-scissor cutout.
## Applies to the resolved submodel and all of its descendants (e.g. cabin instrument
## backlight groups). Submodel names alone are not unique across the tree, so paths
## are resolved via [method E3DModel.get_node_or_null].
@export var force_alpha_submodel_paths:Array[NodePath] = []:
    set(x):
        if not x == force_alpha_submodel_paths:
            force_alpha_submodel_paths = x
            _dirty = true

## When true, every submodel already flagged [code]material_transparent[/code] gets real alpha
## blending instead of the default alpha-scissor cutout - unlike [member force_alpha_submodel_paths],
## this is not limited to specific named submodels. Opaque submodels are unaffected. Intended for
## self-contained models (e.g. a cabin interior) where alpha-scissor's crisp cutout looks wrong
## across the board (glass, instrument backlight glow, ...), unlike mixed-purpose exterior content.
@export var force_alpha:bool = false:
    set(x):
        if not x == force_alpha:
            force_alpha = x
            _dirty = true

## Largest texture size (in pixels) the model's materials load - larger DDS textures drop their top
## mipmap levels. [code]0[/code] takes the project's [code]maszyna/import/dds_max_texture_size[/code].
@export var max_texture_size:int = 0:
    set(x):
        if not x == max_texture_size:
            max_texture_size = x
            _dirty = true

# Probably instancer should be set project-wide
@export var instancer = Instancer.NODES:
    set(x):
        if not x == instancer:
            instancer = x
            _dirty = true

var submodels_aabb:AABB = AABB()
var editable_in_editor:bool = false:
    set(x):
        if not editable_in_editor == x:
            editable_in_editor = x
            _dirty = true


func _get_aabb() -> AABB:
    return submodels_aabb


func _process(_delta: float) -> void:
    if Engine.is_editor_hint():
        if _dirty:
            _dirty = false
            _process_dirty(_delta)


func _process_dirty(_delta: float) -> void:
    reload()


## Reloads the configured E3D model and recreates the server instance using the selected instancer.
func reload() -> void:
    if _proxy:
        _dirty = false
        # null while the owner has not built it (a scenery model out of the streaming's range)
        _model = E3DRenderingServer.instance_get_model(_rid)
        submodels_aabb = E3DModelTool.get_aabb(_model) if _model else AABB()
        update_gizmos()
        E3DRenderingServer.instance_set_instancer(_rid, _get_server_instancer())
        return
    if is_inside_tree() and (model or model_filename):
        _dirty = false
        _free_instance()

        if model:
            _model = model
        else:
            _model = E3DModelManager.load_model(data_path, model_filename)
        if _model:
            lights_state = _merge_lights_state(lights_state)
            submodels_aabb = E3DModelTool.get_aabb(_model)
            _create_instance()


func _ready() -> void:
    # _process only reloads a model edited in the editor; in game it would run for every model
    set_process(Engine.is_editor_hint())
    reload()


func _enter_tree() -> void:
    # the model file and its materials are read again - a model handed over as `model` keeps
    # itself, its materials do not
    GameDataServer.data_reload_requested.connect(reload)
    if _model or _proxy:
        _create_instance()


func _exit_tree() -> void:
    GameDataServer.data_reload_requested.disconnect(reload)
    _free_instance()


func _notification(what: int) -> void:
    match what:
        NOTIFICATION_TRANSFORM_CHANGED:
            if _rid.is_valid() and is_inside_tree():
                E3DRenderingServer.instance_set_transform(_rid, global_transform)
        NOTIFICATION_VISIBILITY_CHANGED:
            if _rid.is_valid():
                E3DRenderingServer.instance_set_visible(_rid, is_visible_in_tree())


## The [E3DRenderingServer] instance of the model, empty until it is created
func get_e3d_instance() -> RID:
    return _rid


## Makes the node stand for an instance somebody else owns - a scenery's model: the node stands
## where the instance does, moving, hiding or showing it as nodes ("Edit E3D") changes that
## instance, and the node neither creates nor frees it. [member data_path], [member model_filename]
## and [member skins] show what it is drawn of. Called before the node enters the tree.
func set_e3d_instance(instance: RID) -> void:
    _proxy = true
    _rid = instance
    top_level = true
    transform = E3DRenderingServer.instance_get_transform(instance)
    data_path = E3DRenderingServer.instance_get_data_path(instance)
    model_filename = E3DRenderingServer.instance_get_model_filename(instance)
    skins = Array(E3DRenderingServer.instance_get_skins(instance))


func is_e3d_loaded() -> bool:
    return _e3d_loaded


## Spawn rate multiplier of the model's particle emitters, as the engine state drives it
## (see [method E3DRenderingServer.instance_set_smoke_intensity])
func set_smoke_intensity(intensity:float) -> void:
    if _rid.is_valid():
        E3DRenderingServer.instance_set_smoke_intensity(_rid, intensity)


## "Edit E3D" shows a proxy's instance as nodes whatever it is drawn with - its owner draws it OPTIMIZED
func _get_server_instancer() -> int:
    if editable_in_editor and (_proxy or instancer == Instancer.NODES):
        return Instancer.EDITABLE_NODES
    return instancer


func _create_instance() -> void:
    var server_instancer: int = _get_server_instancer()
    if _proxy:
        E3DRenderingServer.instance_attach_object_instance_id(_rid, get_instance_id())
        E3DRenderingServer.instance_set_instancer(_rid, server_instancer)
        # the node moved is the instance moved
        set_notify_transform(true)
        _e3d_loaded = true
        e3d_loaded.emit()
        return
    _rid = E3DRenderingServer.instance_create(_model, server_instancer, instance_kind)
    E3DRenderingServer.instance_set_options(
        _rid, data_path, PackedStringArray(skins), exclude_node_names, force_alpha, force_alpha_submodel_paths,
        max_texture_size
    )
    E3DRenderingServer.instance_attach_object_instance_id(_rid, get_instance_id())
    E3DRenderingServer.instance_set_scenario(_rid, get_world_3d().scenario)
    E3DRenderingServer.instance_set_transform(_rid, global_transform)
    E3DRenderingServer.instance_set_visible(_rid, is_visible_in_tree())
    E3DRenderingServer.instance_set_layer_mask(_rid, layers)
    E3DRenderingServer.instance_set_lights_state(_rid, lights_state)
    E3DRenderingServer.instance_set_lights_dimmed(_rid, lights_dimmed, lights_dimmed_multiplier)
    E3DRenderingServer.instance_build(_rid)
    e3d_instance_created.emit(_rid)
    # the one place the submodels come into being: every rebuild, and every return to the tree
    _e3d_loaded = true
    e3d_loaded.emit()
    # OPTIMIZED renders through the server and needs the transform; NODES follows its own nodes,
    # but a particle emitter of the model is owned by the server either way and spawns where the
    # server last saw the instance. Anything else would pay a script call per moved model per frame.
    set_notify_transform(server_instancer == Instancer.OPTIMIZED or _model.smoke_sources.size() > 0)


## The one place the submodels are freed - a reload, and every exit from the tree ("Edit FIZ"
## re-adding a vehicle): whoever keeps nodes of the model hears it first (FINDINGS.md 2026-09-30)
func _free_instance() -> void:
    if _rid.is_valid():
        _e3d_loaded = false
        e3d_loading.emit()
        if _proxy:
            # drawn by its owner as its owner draws it - where the node left it
            E3DRenderingServer.instance_set_instancer(_rid, E3DRenderingServer.INSTANCER_OPTIMIZED)
            E3DRenderingServer.instance_attach_object_instance_id(_rid, 0)
            return
        E3DRenderingServer.instance_free(_rid)
        _rid = RID()


func _merge_lights_state(new_state: Dictionary[String, bool]) -> Dictionary[String, bool]:
    var state: Dictionary[String, bool] = {}
    if _model:
        for light_name in _model.lights.keys():
            state[light_name] = false
    state.merge(new_state, true)
    if _model:
        for light_name: String in new_state.keys():
            if not _model.lights.has(light_name):
                state.erase(light_name)
    state.sort()
    return state
