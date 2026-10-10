extends RefCounted
class_name CabinLogic

## The logic of a vehicle's cabs, attached to the vehicle in CabinSystem
## (vehicle_attach_cab_logic()). It registers the control handlers of one cabin in CabinSystem;
## CabinSystem registers it for the cabin the vehicle's driver sits in and moves it along when the
## driver changes cabins.
## It needs no 3D cab: whoever drives - the player by keys and mouse, the AI - reports
## manipulations with CabinSystem.act(). The player's hand on a control - a key, a click on its
## widget - is one of press(), release(), increase(), decrease(), which decide what the control
## does with it as the cab has it. The original's is LegacyCabinLogic.

## The driver's hand on a control, as a key moves it: press() and increase()/decrease()
enum Gesture { PRESS, INCREASE, DECREASE }


## Registers the handlers of the vehicle's `cabin` (a VehicleServer cabin)
func register(_vehicle_rid:RID, _cabin:RID) -> void:
    pass


func unregister() -> void:
    pass


## The player's keys of the cab's controls (MaszynaPlayer._unhandled_input)
func input(_event:InputEvent) -> void:
    pass


## The driver's hand pushes the control: a button is pressed, a two-position switch flipped
func press(_control_id:StringName) -> void:
    pass


## The driver's hand lets the control go: a push button springs back, a held lever stops
func release(_control_id:StringName) -> void:
    pass


## The driver's hand moves the control one position (or, held, a knob) up
func increase(_control_id:StringName) -> void:
    pass


## The driver's hand moves the control one position (or, held, a knob) down
func decrease(_control_id:StringName) -> void:
    pass


## The input action whose key makes `gesture` on the control in this cab; empty when no key does
func get_action(_control_id:StringName, _gesture:Gesture) -> StringName:
    return &""
