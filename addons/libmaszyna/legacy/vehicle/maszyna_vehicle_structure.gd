@tool
extends Resource
class_name MaszynaVehicleStructure

## What a vehicle's `.mmd` says it is built from, read once and cached on disk.
##
## Everything here is a property of the vehicle *type*, so it is shared by every vehicle of that
## data_path/file_name/skin - and of that name, when the MMD names its (p1): its appearance (the models the MMD names, the resolved skin slots, the
## submodels that move), the models of its cargo and the cab. What says which *instance* a vehicle is - its train_id, its speed, who occupies it,
## where it stands - is not here; it is applied to the vehicle that is built from this.
##
## Reading an MMD is the expensive part and a scenery repeats the same file across a whole
## trainset, so this turns an O(vehicle count) parse into O(distinct types). The nodes themselves
## are cheap and are built per vehicle, rather than packed into a PackedScene and instantiated:
## a vehicle is not a scene, it is a model plus a cab plus a physics handle.

## Normalized, with the leading slash MaterialManager's search path needs.
@export var data_path:String = ""

## The base name the vehicle's .fiz, .mmd and .e3d share - what the `.scn` calls the mmdfile.
@export var file_name:String = ""

## The MMD's `loads:` block: a model per cargo this vehicle can carry, by the cargo's own name.
## A vehicle that declares none still shows its load - the model is then simply named after the
## cargo (TDynamicObject::LoadMMediaFile_mdload(), DynObj.cpp:7195).
@export var load_models:Dictionary[String, String] = {}

## What the vehicle looks like - its models, skins and the submodels that move
## (RailVehicleRenderingServer draws it from this)
@export var appearance:RailVehicleAppearance = null

## coupleradapter: - the adapter the vehicle hands a neighbour of another coupler type
## (MmdCabinInstancer.parse_coupler_adapter()); empty without the key, the original's own then
@export var coupler_adapter:Dictionary = {}

## The cab is genuinely a tree of widgets, so it stays a scene of its own.
@export var cabin_scene:PackedScene = null

## The cabins the MMD defines a cab for (MmdCabinInstancer.parse_cabin_kinds()) - the ones the
## vehicle gets, and can be entered by
@export var cabin_kinds:Array[RailVehicleCabinKind.Kind] = []
