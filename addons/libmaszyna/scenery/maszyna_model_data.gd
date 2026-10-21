@tool
extends ModelData
class_name MaszynaModelData

## Scenery "node model": the core's ModelData, the directory its E3D model's textures are found in
## and the `lights` the node declares - built by MaszynaSceneryProvider as a MaszynaE3DRenderingServer
## instance, not a node. A lit model or one a `lights` event is aimed at is a signal head
## (MaszynaSceneryProvider.assign_signal_head_kinds()).

## The model renderer's backend of MaSzyna's E3D models (ModelData.model_provider)
const MODEL_PROVIDER:StringName = &"maszyna"

@export var data_path:String = ""
## The node's `lights` list, one mode per light in Light_On00..07 order (MaszynaE3DRenderingServer.LightMode
## plus an optional fraction carrying the light's own darkness threshold)
@export var lights:PackedFloat32Array = []
