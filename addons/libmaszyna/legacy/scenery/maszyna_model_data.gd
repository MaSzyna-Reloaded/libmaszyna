@tool
extends Resource
class_name MaszynaModelData

## Scenery "node model" - built by SceneryInstancer as an E3DRenderingServer instance, not a node.

## The node's name, empty for "none"; a signal head is registered under it
@export var name:String = ""
@export var data_path:String = ""
@export var model_filename:String = ""
@export var skins:PackedStringArray = []
@export var position:Vector3 = Vector3.ZERO
## Euler angles (radians), same order as Node3D.rotation
@export var rotation:Vector3 = Vector3.ZERO
@export var range_min:float = 0.0
## Visible up to this distance, 0 - no limit
@export var range_max:float = 0.0
## The node's `lights` list, one mode per light in Light_On00..07 order (E3DRenderingServer.LightMode
## plus an optional fraction carrying the light's own darkness threshold)
@export var lights:PackedFloat32Array = []
## The node's `lightcolors` list, in the same order; a negative colour is the data's "-1", which
## leaves the colour the model carries alone
@export var light_colors:PackedColorArray = []
## Set when the model is a signal head - a lit model or one a `lights` event is aimed at
## (see SceneryInstancer.assign_signal_head_kinds())
@export var signal_head_kind:SignalHeadKind = null
