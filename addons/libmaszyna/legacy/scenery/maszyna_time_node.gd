@tool
extends Node
class_name MaszynaTimeNode

## Marker produced by a scenery's "time:" token; MaszynaSceneryNode applies it after a load.
## Original engine's deserialize_time() (simulationstateserializer.cpp) only ever carried an
## HH:MM scenario clock; sunrise/sunset fields it also used to read are dead ("no longer used,
## calculated dynamically" per that same function's own comment), so start_time is the only
## thing worth porting.
@export var start_time:float = 12.0
