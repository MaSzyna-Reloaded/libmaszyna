@tool
extends MaszynaVehicleModelElementPatch
class_name MaszynaCabControlPatch

## A control of a cab's model (an element of its MaszynaVehicleModelPatch), as an MMD line defines one
## ("label: submodel rot scale offset", MmdCabinInstancer) - the wiper switch of a cab whose MMD has
## none, a handle that turns a wiper by hand (reloaded_manualwiper<N>); one of a label the MMD has is
## left out.

## The MMD's label of it: "wipers_sw", "reloaded_manualwiper1"
@export var label:String = ""
## Its submodel, by its path in the cab model's tree: "banan/PODLOGA_GLOWNA/mech_wyc_raczka01"
@export var submodel:String = ""
## How it moves - "rot", "mov" - and how far a unit of its value (an MMD's scale and offset): a
## turn's share, a handle's of its wiper's whole sweep
@export_enum("rot", "mov") var animation_type:String = "rot"
@export var scale:float = 0.0
@export var offset:float = 0.0
## The turn about its up at each whole value from 0 [deg] - a switch whose positions do not follow
## one turn (a ČD 181's wiper switch: 0 up, 1 left, 2 right); none, the scale's
@export var position_angles:PackedFloat32Array = []
## Which way a drag moves it up, per mouse axis (x: +1 right, y: +1 down) - ZERO follows its grip on
## screen (BaseCabinTool3D.mouse_drag_signs)
@export var mouse_drag_signs:Vector2 = Vector2.ZERO
