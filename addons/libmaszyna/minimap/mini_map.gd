extends Control

@export var track_color: Color = Color.WHITE
@export var label_color: Color = Color.YELLOW
@export var switch_active_color: Color = Color.YELLOW
@export var background_color: Color = Color(0, 0, 0, 0.5)
@export var zoom: float = 1.0:
    set(v):
        zoom = v
        queue_redraw()
@export var view_offset: Vector2 = Vector2.ZERO:
    set(v):
        view_offset = v
        queue_redraw()
@export var follow_camera_rotation: bool = true:
    set(v):
        follow_camera_rotation = v
        queue_redraw()

var _font: Font = ThemeDB.fallback_font
const TRACK_SWITCH_HANDLE_SCRIPT = preload("./switch_handle.gd")
var _switch_handles: Dictionary = {} # RID -> Control
@onready var _ui_container = $UIContainer

var _last_cam_pos: Vector2 = Vector2.INF
var _last_cam_rot: float = 0.0
var _last_zoom: float = 0.0
var _last_view_offset: Vector2 = Vector2.ZERO
var _cached_visible_rect: Rect2
var _cached_tracks:Array[RID] = []

func _ready() -> void:
    mouse_filter = Control.MOUSE_FILTER_STOP
    clip_contents = true

    resized.connect(_sync_switch_handles)

    $UIContainer/BtnZoomIn.pressed.connect(func(): zoom *= 1.2)
    $UIContainer/BtnZoomOut.pressed.connect(func(): zoom /= 1.2)
    $UIContainer/BtnReset.pressed.connect(func(): zoom = 1.0; view_offset = Vector2.ZERO)

func _enter_tree() -> void:
    TrackServer.switch_active_track_changed.connect(_on_switch_changed)
    TrackServer.topology_changed.connect(_on_topology_changed)
    VehicleServer.vehicle_moved.connect(_on_vehicle_moved)

func _exit_tree() -> void:
    TrackServer.switch_active_track_changed.disconnect(_on_switch_changed)
    TrackServer.topology_changed.disconnect(_on_topology_changed)
    VehicleServer.vehicle_moved.disconnect(_on_vehicle_moved)

    resized.disconnect(_sync_switch_handles)

func _process(_delta: float) -> void:
    if not is_visible_in_tree(): return

    var cam:Dictionary = _get_cam_data()
    if not cam.pos == _last_cam_pos or not cam.rot == _last_cam_rot or not zoom == _last_zoom or not view_offset == _last_view_offset:
        _last_cam_pos = cam.pos
        _last_cam_rot = cam.rot
        _last_zoom = zoom
        _last_view_offset = view_offset
        _sync_switch_handles()
        queue_redraw()

func _on_vehicle_moved(_vehicle: RID, position: Vector3) -> void:
    if _cached_visible_rect.has_point(Vector2(position.x, position.z)):
        queue_redraw()

func _on_topology_changed() -> void:
    _cached_visible_rect = Rect2() # Invalidate cache
    _sync_switch_handles()
    queue_redraw()

func _on_switch_changed(_rid: RID, _state: int) -> void:
    _cached_visible_rect = Rect2() # Invalidate cache
    _sync_switch_handles()
    queue_redraw()

func _sync_switch_handles() -> void:
    if not is_visible_in_tree(): return
    var cam: Dictionary = _get_cam_data()
    var current_rids: Array = TrackServer.track_get_rids()
    var current_rids_map: Dictionary = {}
    for rid in current_rids: current_rids_map[rid] = true

    for rid in _switch_handles.keys():
        if not rid in current_rids_map:
            _switch_handles[rid].queue_free()
            _switch_handles.erase(rid)

    for rid: RID in current_rids:
        if TrackServer.track_is_switch(rid):
            if not _switch_handles.has(rid):
                var sh: Control = _create_switch_handle(rid)
                add_child(sh)
                _switch_handles[rid] = sh
            var sh: Control = _switch_handles[rid]
            var common_idx: int = TrackServer.track_get_common_endpoint_index(rid)
            var endpoints: PackedVector3Array = TrackServer.track_get_endpoints(rid)
            if not common_idx == TrackServer.POINT_NONE and common_idx < endpoints.size():
                sh.position = _world_to_view_centered(endpoints[common_idx], cam.pos, cam.rot) - sh.size * 0.5
                sh.visible = true
                sh.modulate = Color.WHITE if TrackServer.switch_get_active_track(rid) == TrackServer.TRACK_COMMON else Color.GRAY
            else:
                sh.visible = false
        elif _switch_handles.has(rid):
            _switch_handles[rid].visible = false

func _create_switch_handle(rid: RID) -> Control:
    var h:Control = TRACK_SWITCH_HANDLE_SCRIPT.new()
    h.track_rid = rid
    h.viewer = self
    h.size = Vector2(15, 15)
    h.mouse_filter = Control.MOUSE_FILTER_STOP
    return h
func _get_cam_data() -> Dictionary:
    var cam:Camera3D = get_viewport().get_camera_3d()
    var pos:Vector2 = Vector2.ZERO
    var rot:float = 0.0
    if cam:
        pos = Vector2(cam.global_position.x, cam.global_position.z)
        if follow_camera_rotation:
            rot = cam.global_transform.basis.get_euler().y
    return {"pos": pos, "rot": rot}

func _draw() -> void:
    draw_rect(Rect2(Vector2.ZERO, size), background_color)
    var cam: Dictionary = _get_cam_data()

    var view_size: Vector2 = size / zoom
    var visible_rect: Rect2 = Rect2(cam.pos - view_size * 0.5, view_size).grow(50.0)

    if not visible_rect == _cached_visible_rect:
        _cached_visible_rect = visible_rect
        _cached_tracks = TrackServer.tracks_find_in_aabb(visible_rect)

    for rid in _cached_tracks:
        var is_switch: bool = TrackServer.track_is_switch(rid)
        var curve1: TrackCurve = TrackServer.track_get_curve(rid, TrackServer.TRACK_COMMON)
        var curve2: TrackCurve = TrackServer.track_get_curve(rid, TrackServer.TRACK_DIVERGING)
        var switch_track: TrackServer.SwitchTrack = TrackServer.switch_get_active_track(rid)
        var track_name: String = TrackServer.track_get_name(rid)

        _draw_track_curves(curve1, curve2, switch_track, is_switch, cam.pos, cam.rot)
        _draw_track_label(track_name, curve1, cam.pos, cam.rot)
    _draw_trains(visible_rect, cam.pos, cam.rot)

func _draw_trains(visible_rect: Rect2, cam_pos: Vector2, cam_rot: float) -> void:
    if not Engine.is_editor_hint():
        var vehicles: Array[RID] = RailVehicleServer.vehicle_get_rids_in_rect(visible_rect)
        for vehicle: RID in vehicles:
            var vehicle_position:Vector3 = RailVehicleServer.vehicle_get_transform(vehicle).origin
            var vehicle_name: String = VehicleServer.vehicle_get_name(vehicle)
            var view_pos: Vector2 = _world_to_view_centered(vehicle_position, cam_pos, cam_rot)
            var square_size: float = 10.0
            draw_rect(Rect2(view_pos - Vector2(square_size / 2, square_size / 2), Vector2(square_size, square_size)), Color.GREEN)

            var font_size: int = 12
            draw_string(_font, view_pos + Vector2(square_size * 0.7, font_size * 0.3), vehicle_name, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, Color.GREEN)

func _draw_track_curves(
    curve1: TrackCurve,
    curve2: TrackCurve,
    switch_track: TrackServer.SwitchTrack,
    is_switch: bool,
    cam_pos: Vector2,
    cam_rot: float
) -> void:
    if curve1:
        _draw_curve(curve1, switch_active_color if is_switch and switch_track == TrackServer.TRACK_COMMON else track_color, cam_pos, cam_rot)
    if curve2:
        _draw_curve(curve2, switch_active_color if is_switch and switch_track == TrackServer.TRACK_DIVERGING else track_color, cam_pos, cam_rot)

func _draw_track_label(track_name: String, curve1: TrackCurve, cam_pos: Vector2, cam_rot: float) -> void:
    if not track_name or not curve1: return
    var view_pos:Vector2 = _world_to_view_centered(_sample_bezier(curve1, 0.5), cam_pos, cam_rot)
    var font_size:int = 12
    draw_string(_font, view_pos - _font.get_string_size(track_name, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size) * 0.5 + Vector2(0, font_size * 0.25), track_name, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, label_color)

func _draw_curve(curve: TrackCurve, color: Color, cam_pos: Vector2, cam_rot: float) -> void:
    if not curve: return
    var points: PackedVector2Array = []
    for i in range(17):
        points.append(_world_to_view_centered(_sample_bezier(curve, float(i) / 16.0), cam_pos, cam_rot))
    draw_polyline(points, color, 1.0, true)

func _sample_bezier(curve: TrackCurve, t: float) -> Vector3:
    var p0:Vector3 = curve.p1
    var p1:Vector3 = curve.p1 + curve.c1
    var p2:Vector3 = curve.p2 + curve.c2
    var p3:Vector3 = curve.p2
    return p0.lerp(p1, t).lerp(p1.lerp(p2, t), t).lerp(p1.lerp(p2, t).lerp(p2.lerp(p3, t), t), t)

func _view_to_world_centered(view_pos: Vector2, cam_pos: Vector2, cam_rot: float) -> Vector3:
    var rel_pos:Vector2 = (view_pos - size * 0.5 - view_offset) / zoom
    if not cam_rot == 0.0: rel_pos = rel_pos.rotated(-cam_rot)
    var p:Vector2 = rel_pos + cam_pos
    return Vector3(p.x, 0, p.y)

func _world_to_view_centered(pos: Vector3, cam_pos: Vector2, cam_rot: float) -> Vector2:
    var rel_pos:Vector2 = Vector2(pos.x, pos.z) - cam_pos
    if not cam_rot == 0.0: rel_pos = rel_pos.rotated(cam_rot)
    return rel_pos * zoom + size * 0.5 + view_offset
