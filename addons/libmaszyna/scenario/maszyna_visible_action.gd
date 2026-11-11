@tool
extends ScenarioEventAction
class_name MaszynaVisibleAction

## The original's `visible` event (visible_event, Event.cpp:2015-2064): hides its models, tracks and
## traction spans, or shows them again.

## The models' ModelRenderingServer instances
var instances:Array[RID] = []
## The tracks' TrackRenderingServer renders
var track_renders:Array[RID] = []
## The traction spans' TractionRenderingServer handles
var tractions:Array[RID] = []
@export var visible:bool = true


func _run(_event:RID, _activator:RID) -> void:
    for instance:RID in instances:
        ModelRenderingServer.instance_set_visible(instance, visible)
    for track_render:RID in track_renders:
        TrackRenderingServer.track_set_visible(track_render, visible)
    for traction:RID in tractions:
        TractionRenderingServer.set_traction_visible(traction, visible)
