@tool
extends VBoxContainer

## Live view of SceneryStreamingServer: what a scenery registered, what is actually built around
## the camera right now, and how much of the plan is still waiting for the per-frame budget - and
## what the process uses. Shown in the game's HUD and in the editor's bottom panel (the scenery
## streaming plugin); refreshed only while it is visible.

## Seconds between refreshes - the numbers only change with a streaming pass
const REFRESH_INTERVAL:float = 0.25
const BYTES_PER_GB:float = 1024.0 * 1024.0 * 1024.0

var _rows:Dictionary[String, Label] = {}
var _timer:Timer = Timer.new()


func _ready() -> void:
    for caption:String in [
        "Camera", "Camera chunk", "Draw distance", "Chunks", "Chunks in range",
        "Registered", "Streamed in", "Pending builds", "Built ahead", "Pending nearby", "Nearby ready",
        "Pending clears", "Planning", "Builds/s", "Budget", "Filling", "Last pass", "Owners",
        "Supplied cells", "Provides", "Withdraws", "Main thread", "Scenery lights",
        "Resident memory", "Godot static", "Objects", "Lazy resources",
    ]:
        _rows[caption] = _add_row(caption)
    _timer.wait_time = REFRESH_INTERVAL
    _timer.timeout.connect(_refresh)
    add_child(_timer)
    _refresh()
    if is_visible_in_tree():
        _timer.start()


func _notification(what:int) -> void:
    if what == NOTIFICATION_VISIBILITY_CHANGED and _timer.is_inside_tree():
        if is_visible_in_tree():
            _refresh()
            _timer.start()
            return
        _timer.stop()


func _refresh() -> void:
    var statistics:Dictionary = SceneryStreamingServer.streaming_get_statistics()
    var camera_position:Vector3 = statistics["camera_position"]
    var camera_chunk:Vector2i = statistics["camera_chunk"]
    var registered:int = statistics["registered"]
    var streamed:int = statistics["streamed"]

    _rows["Camera"].text = (
        "%.0f, %.0f, %.0f" % [camera_position.x, camera_position.y, camera_position.z]
        if statistics["has_camera"] else tr("none - nothing is streamed")
    )
    _rows["Camera chunk"].text = "%d, %d" % [camera_chunk.x, camera_chunk.y]
    _rows["Draw distance"].text = tr("%.0f m (chunk %.0f m)") % [statistics["draw_distance"], statistics["chunk_size"]]
    _rows["Chunks"].text = str(statistics["chunks"])
    _rows["Chunks in range"].text = str(statistics["active_chunks"])
    _rows["Registered"].text = str(registered)
    _rows["Streamed in"].text = (
        "%d (%.1f%%)" % [streamed, 100.0 * float(streamed) / float(registered)] if registered
        else str(streamed)
    )
    _rows["Pending builds"].text = str(statistics["pending_builds"])
    # ahead of their range, with the little left of a frame once nothing in range waits
    _rows["Built ahead"].text = tr("%d waiting") % statistics["pending_prefetches"]
    _rows["Pending nearby"].text = str(statistics["pending_nearby"])
    _rows["Nearby ready"].text = str(statistics["nearby_ready"])
    _rows["Pending clears"].text = str(statistics["pending_clears"])
    _rows["Planning"].text = str(statistics["planning"])
    _rows["Builds/s"].text = str(statistics["build_rate"])
    _rows["Budget"].text = "%d ms/frame" % statistics["budget_msec"]
    _rows["Last pass"].text = "%d ms" % statistics["plan_msec"]
    _rows["Owners"].text = str(statistics["owners"])
    _rows["Filling"].text = str(statistics["filling"])
    _rows["Supplied cells"].text = "%d (%d providers, %d waiting)" % [
        statistics["supplied_cells"], statistics["providers"], statistics["pending_provides"],
    ]
    _rows["Provides"].text = tr("%.1f ms/s, longest %.1f ms") % [
        statistics["provide_msec"], statistics["provide_max_msec"],
    ]
    _rows["Withdraws"].text = tr("%.1f ms/s") % statistics["withdraw_msec"]
    # what each owner's builds and clears took of the main thread in the last second, and the
    # longest single one - a build longer than the budget cannot be cut
    var owner_msec:Dictionary = statistics["owner_msec"]
    var owner_max_msec:Dictionary = statistics["owner_max_msec"]
    var lines:PackedStringArray = []
    for owner_name:String in owner_msec:
        lines.append(tr("%s: %.1f ms/s, longest %.1f ms") % [owner_name, owner_msec[owner_name], owner_max_msec[owner_name]])
    _rows["Main thread"].text = "\n".join(lines)

    # Real (spot/omni) lights of the streamed scenery models; "synth" are the ones the street lamp
    # quirk derived for models that light the scene without declaring a spotlight submodel
    var lights:Dictionary = E3DRenderingServer.light_get_statistics()
    _rows["Scenery lights"].text = tr("%d lit / %d (%d spot, %d omni, %d synth)") % [
        lights["lit"], lights["spot"] + lights["omni"], lights["spot"], lights["omni"],
        lights["synthesized"],
    ]

    # what the process uses (SceneryLoadMeasurement.print_process())
    _rows["Resident memory"].text = "%.2f GB" % (ProcessMemory.get_resident_bytes() / BYTES_PER_GB)
    _rows["Godot static"].text = "%.2f GB" % (Performance.get_monitor(Performance.MEMORY_STATIC) / BYTES_PER_GB)
    _rows["Objects"].text = tr("%d (%d resources, %d nodes)") % [
        Performance.get_monitor(Performance.OBJECT_COUNT),
        Performance.get_monitor(Performance.OBJECT_RESOURCE_COUNT),
        Performance.get_monitor(Performance.OBJECT_NODE_COUNT),
    ]
    var lazy:Dictionary = ResourceLazyLoader.resource_get_statistics()
    _rows["Lazy resources"].text = tr("%d resident / %d registered, %d loads") % [
        lazy["resident"], lazy["registered"], lazy["loads"],
    ]


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
