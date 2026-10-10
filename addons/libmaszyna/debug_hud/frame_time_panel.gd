extends VBoxContainer

## Where a frame actually goes. The F3 debug menu reports only the renderer's own CPU time
## (RenderingServer.viewport_get_measured_render_time_cpu) and the GPU time, so a frame that is
## slow in scripts or physics shows up there as a gap between "Total" and CPU+GPU with nothing to
## explain it. These are the monitors that fill that gap in.

## Seconds between refreshes
const REFRESH_INTERVAL:float = 0.25

var _rows:Dictionary[String, Label] = {}
var _elapsed:float = 0.0


func _ready() -> void:
    for caption:String in [
        "Frame", "FPS", "Process", "Physics", "Render CPU", "Render GPU", "Unaccounted",
        "Physics steps", "Objects", "Draw calls", "Primitives", "Video memory",
    ]:
        _rows[caption] = _add_row(caption)
    _refresh()


func _process(delta:float) -> void:
    _elapsed += delta
    if _elapsed < REFRESH_INTERVAL:
        return
    _elapsed = 0.0
    _refresh()


func _refresh() -> void:
    var fps:float = Performance.get_monitor(Performance.TIME_FPS)
    var frame:float = 1000.0 / fps if fps > 0.0 else 0.0
    var process:float = Performance.get_monitor(Performance.TIME_PROCESS) * 1000.0
    var physics:float = Performance.get_monitor(Performance.TIME_PHYSICS_PROCESS) * 1000.0
    var viewport:RID = get_viewport().get_viewport_rid()
    var render_cpu:float = (
        RenderingServer.viewport_get_measured_render_time_cpu(viewport)
        + RenderingServer.get_frame_setup_time_cpu()
    )
    var render_gpu:float = RenderingServer.viewport_get_measured_render_time_gpu(viewport)

    _rows["Frame"].text = "%.1f ms" % frame
    _rows["FPS"].text = "%.0f" % fps
    _rows["Process"].text = "%.1f ms" % process
    _rows["Physics"].text = "%.1f ms" % physics
    _rows["Render CPU"].text = "%.1f ms" % render_cpu
    _rows["Render GPU"].text = "%.1f ms" % render_gpu
    # what none of the monitors above claims: engine internals, resource work, driver stalls
    _rows["Unaccounted"].text = "%.1f ms" % maxf(frame - process - physics - render_cpu, 0.0)
    # physics ticks Godot runs per rendered frame to catch up - each one is a full simulation step
    _rows["Physics steps"].text = tr("%.1f (max %d)") % [
        minf(frame / (1000.0 / Engine.physics_ticks_per_second), Engine.max_physics_steps_per_frame),
        Engine.max_physics_steps_per_frame,
    ]
    _rows["Objects"].text = str(int(Performance.get_monitor(Performance.OBJECT_NODE_COUNT)))
    _rows["Draw calls"].text = str(int(Performance.get_monitor(Performance.RENDER_TOTAL_DRAW_CALLS_IN_FRAME)))
    _rows["Primitives"].text = str(int(Performance.get_monitor(Performance.RENDER_TOTAL_PRIMITIVES_IN_FRAME)))
    _rows["Video memory"].text = "%.0f MB" % (Performance.get_monitor(Performance.RENDER_VIDEO_MEM_USED) / 1048576.0)


func _add_row(caption:String) -> Label:
    var row := HBoxContainer.new()
    var caption_label := Label.new()
    caption_label.text = caption
    caption_label.custom_minimum_size = Vector2(130, 0)
    var value_label := Label.new()
    value_label.size_flags_horizontal = Control.SIZE_EXPAND_FILL
    value_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
    row.add_child(caption_label)
    row.add_child(value_label)
    add_child(row)
    return value_label
