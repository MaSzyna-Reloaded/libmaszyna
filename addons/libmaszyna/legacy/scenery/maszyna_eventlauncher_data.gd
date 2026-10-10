@tool
extends Resource
class_name MaszynaEventLauncherData

## Scenery `node <rmax> <rmin> <name> eventlauncher <x> <y> <z> <radius> <key> <deltatime>
## <event1> [<event2>] [condition <memcell> <text> <value1> <value2>] [traintriggered] end`
## (TEventLauncher::Load(), EvLaunch.cpp:52-159). Tokens are lower case, as the original reads
## them.

@export var name:String = ""
@export var position:Vector3 = Vector3.ZERO
## Metres, negative for anywhere
@export var radius:float = -1.0
## One character, `radio_call1`/`radio_call3`, or a key code; empty for none
@export var key:String = ""
## Negative: fires every |deltatime| seconds; positive: once at HHMM; 0: only by its key
## (EvLaunch.cpp:136-157)
@export var delta_time:float = 0.0
@export var event1:String = ""
## Fired with Shift, empty for none
@export var event2:String = ""
## `<memcell> <text> <value1> <value2>`, empty for no condition
@export var condition:PackedStringArray = []
## Measured to the train rather than to the camera (scene.cpp:129-137)
@export var train_triggered:bool = false
