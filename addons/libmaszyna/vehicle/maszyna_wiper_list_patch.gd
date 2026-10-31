@tool
extends MaszynaVehicleModelElementPatch
class_name MaszynaWiperListPatch

## The wiper switch of a FIZ of no WiperList: its positions, as the list's rows (FizTrainWipersParser)
## - the vehicle's wipers are then the switch's, not driven by hand.

## How far the wipers turn [deg] (WiperList: Angle=)
@export var angle:float = 0.0
## The switch's positions, the first its off one
@export var positions:Array[RailVehicleWiperListItem] = []
