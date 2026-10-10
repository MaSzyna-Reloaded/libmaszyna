@tool
extends Control

var track_rid: RID
var viewer: Control
var _hovering: bool = false

func _draw() -> void:
    var radius:float = size.x * 0.5
    var color:Color = Color.WHITE
    if _hovering:
        color = Color.YELLOW

    # Draw a circle with a border
    draw_circle(Vector2(radius, radius), radius, color)
    draw_arc(Vector2(radius, radius), radius, 0, TAU, 32, Color.BLACK, 1.0)

func _notification(what: int) -> void:
    match what:
        NOTIFICATION_MOUSE_ENTER:
            _hovering = true
            queue_redraw()
        NOTIFICATION_MOUSE_EXIT:
            _hovering = false
            queue_redraw()

func _gui_input(event: InputEvent) -> void:
    if event is InputEventMouseButton:
        if event.button_index == MOUSE_BUTTON_LEFT and event.pressed:
            _toggle_switch()
            accept_event()

func _toggle_switch() -> void:
    var active_track: TrackServer.SwitchTrack = TrackServer.switch_get_active_track(track_rid)
    var new_active_track:TrackServer.SwitchTrack = (
        TrackServer.TRACK_COMMON
        if active_track == TrackServer.TRACK_DIVERGING
        else TrackServer.TRACK_DIVERGING
    )
    TrackServer.switch_set_active_track(track_rid, new_active_track)
    viewer.queue_redraw()

func _get_tooltip(_at_position: Vector2) -> String:
    var active_track: TrackServer.SwitchTrack = TrackServer.switch_get_active_track(track_rid)
    var name: String = TrackServer.track_get_name(track_rid)
    var state_str:String = "Straight" if active_track == TrackServer.TRACK_COMMON else "Diverging"
    return "Switch: %s\nState: %s" % [name or "unnamed", state_str]
