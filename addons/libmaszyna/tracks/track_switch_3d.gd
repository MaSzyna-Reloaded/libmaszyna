@tool
extends TrackNormal3D
class_name TrackSwitch3D

signal switching_started(from_track: int, to_track: int)
signal switching_finished(active_track: int)

func _init() -> void:
    type = TrackServer.TRACK_SWITCH

@export var diverging_curve:TrackCurve:
    set(x):
        if diverging_curve == x:
            return
        if diverging_curve and is_inside_tree():
            diverging_curve.changed.disconnect(_mark_dirty)
        diverging_curve = x
        if diverging_curve and is_inside_tree():
            diverging_curve.changed.connect(_mark_dirty)
        _dirty = true

@export var active_track: TrackServer.SwitchTrack = TrackServer.TRACK_COMMON:
    set(value):
        if active_track == value:
            return
        active_track = value
        if _track_rid.is_valid():
            TrackServer.switch_set_active_track(_track_rid, active_track)


func toggle_switch() -> void:
    if not _track_rid.is_valid():
        return
    var next_track:TrackServer.SwitchTrack = (
        TrackServer.TRACK_COMMON
        if active_track == TrackServer.TRACK_DIVERGING
        else TrackServer.TRACK_DIVERGING
    )
    TrackServer.switch_set_active_track(_track_rid, next_track)

func _update_track_curve() -> void:
    TrackServer.track_update_curves(_track_rid, curve, diverging_curve)

func _update_track_data() -> void:
    super._update_track_data()
    TrackServer.switch_set_active_track(_track_rid, active_track)

func _enter_tree():
    super._enter_tree()
    if diverging_curve:
        diverging_curve.changed.connect(_mark_dirty)
    TrackServer.switch_active_track_changed.connect(_on_track_server_switch_active_track_changed)
    TrackServer.switch_movement_started.connect(_on_track_server_switching_started)
    TrackServer.switch_movement_finished.connect(_on_track_server_switching_finished)

func _exit_tree() -> void:
    if diverging_curve:
        diverging_curve.changed.disconnect(_mark_dirty)
    TrackServer.switch_active_track_changed.disconnect(_on_track_server_switch_active_track_changed)
    TrackServer.switch_movement_started.disconnect(_on_track_server_switching_started)
    TrackServer.switch_movement_finished.disconnect(_on_track_server_switching_finished)
    super._exit_tree()

func _get_aabb_points() -> Array[Vector3]:
    var points:Array[Vector3] = super._get_aabb_points()
    if diverging_curve:
        points.append(diverging_curve.p1)
        points.append(diverging_curve.p2)
    return points

func _on_track_server_switching_started(track_rid: RID, from_track: int, to_track: int) -> void:
    if not track_rid == _track_rid:
        return
    switching_started.emit(from_track, to_track)

func _on_track_server_switching_finished(track_rid: RID, active_track_value: int) -> void:
    if not track_rid == _track_rid:
        return
    switching_finished.emit(active_track_value)

func _on_track_server_switch_active_track_changed(track_rid: RID, active_track_value: int) -> void:
    if not track_rid == _track_rid:
        return
    if not self.active_track == active_track_value:
        self.active_track = active_track_value
