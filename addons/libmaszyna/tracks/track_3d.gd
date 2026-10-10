@tool
@abstract
extends VisualInstance3D
class_name Track3D

enum TrackEnvironment {FLAT, BRIDGE, TUNNEL, MOUNTAINS, CANYON, BANK}
enum TrackDamageFlagBit {NONE=0, DESTROYED=128}

var type: TrackServer.TrackType = TrackServer.TRACK_UNKNOWN

@export var track_name: String = "":
    set(x):
        if not track_name == x:
            track_name = x
            _dirty = true

@export var curve:TrackCurve:
    set(x):
        if curve == x:
            return
        if curve and is_inside_tree():
            curve.changed.disconnect(_mark_dirty)
        curve = x
        if curve and is_inside_tree():
            curve.changed.connect(_mark_dirty)
        _dirty = true

@export var environment:TrackEnvironment = TrackEnvironment.FLAT
@export_flags("Destroyed:128") var damage_flag:int = 0
@export var quality_flag:int
## Rail length between joints for the wheel clatter, -1 for none
@export var sound_distance:float = -1.0
@export var friction:float = 0.0
@export var parameters:Dictionary = {}
@export var width: float = 1.5:
    set(value):
        if is_equal_approx(width, value):
            return
        width = value
        _dirty = true


@export_group("Legacy or unused")
@export var length:float = 0.0  # unused, Rainsted fills with mileage of the first point

var _track_rid: RID
var _track_render_rid: RID
var _dirty:bool = false

const DEBOUNCE_TIME = 0.1
var _last_debouce:float

func _enter_tree() -> void:
    if curve:
        curve.changed.connect(_mark_dirty)

func _exit_tree() -> void:
    if curve:
        curve.changed.disconnect(_mark_dirty)

func _mark_dirty():
    _dirty = true

func _ready():
    set_process(true)

func _notification(what):
    match what:
        NOTIFICATION_VISIBILITY_CHANGED:
            if _track_render_rid.is_valid():
                TrackRenderingServer.set_track_visible(_track_render_rid, visible)

func _process(delta: float) -> void:
    if _dirty:
        _dirty = false
        var current_time:float = Time.get_ticks_msec() / 1000.0
        if current_time - _last_debouce > DEBOUNCE_TIME:
            _last_debouce = current_time
            _process_dirty(delta)

func _process_dirty(_delta: float) -> void:
    pass

func _get_aabb_points() -> Array[Vector3]:
    var points: Array[Vector3] = []
    if curve:
        points.append(curve.p1)
        points.append(curve.p2)
    return points

func _get_global_aabb() -> AABB:
    var points:Array[Vector3] = _get_aabb_points()
    if not points:
        return AABB()

    var aabb:AABB = AABB(points[0], Vector3.ZERO)

    for p in points:
        aabb = aabb.expand(p)
    return aabb

func _get_aabb() -> AABB:
    var points:Array = _get_aabb_points().map(to_local)

    if not points:
        return AABB()

    var aabb:AABB = AABB(points[0], Vector3.ZERO)

    for p in points:
        aabb = aabb.expand(p)

    var half_width:float = width * 0.5
    half_width += 1.0  # FIXME: hardcoded / guess

    aabb.position.x -= half_width
    aabb.position.z -= half_width
    aabb.size.x += half_width * 2
    aabb.size.z += half_width * 2
    aabb.position.y -= 0.2  # FIXME: hardcoded / guess
    aabb.size.y += 0.4  # FIXME: hardcoded / guess

    return aabb
