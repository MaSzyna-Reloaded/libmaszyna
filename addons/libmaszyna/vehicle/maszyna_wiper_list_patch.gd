@tool
extends MaszynaVehicleModelElementPatch
class_name MaszynaWiperListPatch

## The wiper switch of a FIZ of no WiperList: its positions, as the list's rows (FizTrainWipersParser)
## - the vehicle's wipers are then the switch's, not driven by hand.

## How far the wipers turn [deg] (WiperList: Angle=)
@export var angle:float = 0.0
## The switch's positions, the first its off one
@export var positions:Array[RailVehicleWiperListItem] = []
## How many wipers it moves where the vehicle's model animates none - its arms part of a static mesh
## (an ED72's): 0, as many as the model animates (MaszynaRailVehicle3DInstancer.apply_wiper_count())
@export var wiper_count:int = 0
