@tool
extends HBoxContainer


@onready var _info_window:Window = $InfoWindow


func _ready() -> void:
    TrackServer.topology_changed.connect(_on_tracks_topology_changed)
    _on_tracks_topology_changed()

func _exit_tree() -> void:
    TrackServer.topology_changed.disconnect(_on_tracks_topology_changed)

func _on_tracks_topology_changed() -> void:
    var btn:Button = $RebuildTopologyButton
    if btn.text.begins_with("*") and not TrackServer.is_topology_changed:
        btn.text = btn.text.substr(2)
    elif not btn.text.begins_with("*") and TrackServer.is_topology_changed:
        btn.text = "* %s" % btn.text

func _on_rebuild_topology_button_pressed() -> void:
    _info_window.show_message("Rebuilding...")
    # Give the UI a frame to show "Rebuilding..." before blocking for rebuild
    await get_tree().process_frame
    TrackServer.topology_rebuild()
    _show_summary()

    _on_tracks_topology_changed()

func _on_view_topology_button_pressed() -> void:
    _show_summary()

func _show_summary() -> void:
    var summary:Dictionary = TrackServer.topology_get_summary()
    var graph_stats:Array[String] = []
    for graph:Dictionary in summary.graphs:
        if graph.num_tracks > 1:
            graph_stats.append(
                "Graph %d: Tracks: %d, Switches: %d, Length: %.2f" % [
                    graph.id, graph.num_tracks, graph.num_switches, graph.total_length
                ])

    var total_orphaned:int = summary.orphaned_tracks_count

    _info_window.show_message(
        "Tracks topology summary:\n\n" + "\n".join(graph_stats) +
        "\n\nOrphaned tracks: " + str(total_orphaned)
    )
