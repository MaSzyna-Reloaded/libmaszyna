@tool
extends MaszynaVehicleModelElementPatch
class_name MaszynaWiperArmPatch

## A wiper arm of a vehicle's model (an element of its MaszynaVehicleModelPatch): turned whole, blade
## and all, about its own point and axis by its own angle (MaszynaE3DRailVehicleRenderingServer) - in
## the order of the FIZ's wipers, wiper 1 first.

## The arm's submodel, by its path in the model's tree: "baza/ostoja/wycieraczka01"
@export var submodel:String = ""
## The point it turns about, in its own frame: its hub - ZERO, its origin, where the model put it there
@export var pivot:Vector3 = Vector3.ZERO
## The axis it turns about, in its own frame: across the glass
@export var axis:Vector3 = Vector3.UP
## How far it turns from where it is parked across its sweep [deg] - signed, the way it goes out
@export var turn:float = 90.0
