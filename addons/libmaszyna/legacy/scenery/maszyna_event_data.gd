@tool
extends Resource
class_name MaszynaEventData

## Scenery `event <name> <type> <delay> <targets> <parameters> [condition ...] [randomdelay <s>]
## [departuredelay <s>] endevent` (basic_event::deserialize(), Event.cpp:289-333). The parameters are kept as they
## stand in the file; what they mean is the type's business (see [MaszynaLegacyEventFactory])

## Lower case, as the original keeps event names (Event.cpp:2202)
@export var name:String = ""
## Lower case (make_event(), Event.cpp:2199-2236)
@export var type:String = ""
## Seconds; a negative delay also queues the event when the scenery starts (Event.cpp:2498)
@export var delay:float = 0.0
## Up to this many seconds more, drawn each time the event is queued (Event.cpp:2407-2411)
@export var random_delay:float = 0.0
## Seconds from the departure of the train that queues the event, NAN for an event that does not
## wait for one (m_delaydeparture, Event.cpp:314-316, 2431-2441)
@export var departure_delay:float = NAN
## Lower case names of what the event is aimed at, "none" left out (Event.cpp:323-333)
@export var targets:PackedStringArray = []
## The type's own tokens
@export var parameters:PackedStringArray = []
## The tokens after `condition` (event_conditions::deserialize(), Event.cpp:181-254)
@export var condition:PackedStringArray = []
## The origin in effect where the event is defined - what a position among its parameters is
## relative to (`putvalues`, Event.cpp:709-712)
@export var origin:Vector3 = Vector3.ZERO
