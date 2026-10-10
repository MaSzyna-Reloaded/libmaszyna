@tool
extends Track3D
class_name TrackNormal3D

func _init() -> void:
    type = TrackServer.TRACK_NORMAL

@export var material1: String = "":
    set(value):
        if material1 == value:
            return
        material1 = value
        _dirty = true

@export var railprofile: String = "default":
    set(value):
        if railprofile == value:
            return
        railprofile = value
        _dirty = true

@export var material2: String = "":
    set(value):
        if material2 == value:
            return
        material2 = value
        _dirty = true

@export var trackbed_material: String = "":
    set(value):
        if trackbed_material == value:
            return
        trackbed_material = value
        _dirty = true

@export var rail_visible: bool = true:
    set(value):
        if rail_visible == value:
            return
        rail_visible = value
        _dirty = true

@export var ballast_visible: bool = true:
    set(value):
        if ballast_visible == value:
            return
        ballast_visible = value
        _dirty = true

@export var tex_length: float = 4.0:
    set(value):
        if is_equal_approx(tex_length, value):
            return
        tex_length = value
        _dirty = true

@export var tex_height: float = 0.0:
    set(value):
        if is_equal_approx(tex_height, value):
            return
        tex_height = value
        _dirty = true

@export var tex_width: float = 0.0:
    set(value):
        if is_equal_approx(tex_width, value):
            return
        tex_width = value
        _dirty = true

@export var tex_slope: float = 0.0:
    set(value):
        if is_equal_approx(tex_slope, value):
            return
        tex_slope = value
        _dirty = true


func _enter_tree():
    super._enter_tree()
    _create_track()
    _update_track_data()
    _dirty = true

func _exit_tree():
    _free_track()
    super._exit_tree()

func _update_track_curve() -> void:
    TrackServer.track_update_curves(_track_rid, curve, null)

func _update_track_data() -> void:
    if not _track_rid.is_valid():
        return

    self.global_position = _get_global_aabb().get_center()
    _update_track_curve()

    TrackServer.track_update(
        _track_rid,
        type,
        track_name,
        width,
    )
    TrackServer.track_update_properties(_track_rid, quality_flag, environment, sound_distance)

func _process_dirty(_delta: float) -> void:
    super._process_dirty(_delta)
    _update_track_data()
    _update_track_rendering()

func _update_track_rendering():
    if not _track_rid or not _track_rid.is_valid() or not _track_render_rid or not _track_render_rid.is_valid() or not curve:
        return

    TrackRenderingServer.set_track_render_options(
        _track_render_rid,
        tex_length,
        tex_height,
        tex_width,
        tex_slope,
        material1,
        material2,
        trackbed_material,
        railprofile,
        rail_visible,
        ballast_visible
    )
    TrackRenderingServer.rebuild_track(_track_render_rid)
    TrackRenderingServer.set_track_scenario(_track_render_rid, get_world_3d().scenario)
    TrackRenderingServer.set_track_visible(_track_render_rid, visible)

func _create_track() -> void:
    _free_track()
    _track_rid = TrackServer.track_create()
    _track_render_rid = TrackRenderingServer.create_track(_track_rid)
    TrackRenderingServer.set_track_scenario(_track_render_rid, get_world_3d().scenario)

func _free_track() -> void:
    if _track_render_rid.is_valid():
        TrackRenderingServer.free_track(_track_render_rid)
    if _track_rid.is_valid():
        TrackServer.track_free(_track_rid)
    _track_rid = RID()
    _track_render_rid = RID()
