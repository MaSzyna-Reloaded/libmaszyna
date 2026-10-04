@tool
extends Resource
class_name MaszynaTrackData


@export var type:int # TrackServer.TrackType
@export var track_name:String = ""
@export var curve:TrackCurve
@export var diverging_curve:TrackCurve
@export var length:float = 0.0
@export var width:float = 1.5
@export var friction:float = 0.0
@export var quality_flag:int = 0
## Rail length between joints for the wheel clatter, -1 for none
@export var sound_distance:float = -1.0
@export var damage_flag:int = 0
@export var environment:int = 0 # Track3D.TrackEnvironment
@export var visible:bool = true
@export var material1:String = ""
@export var material2:String = ""
@export var tex_length:float = 4.0
@export var tex_height:float = 0.0
@export var tex_width:float = 0.0
@export var tex_slope:float = 0.0
@export var railprofile:String = "default"
@export var parameters:Dictionary = {}
## The events the track names for each of its slots, by the slot's key: a slot may name several,
## every `event2 <name>` line one more (Track.cpp:815-858, m_events2.emplace_back)
@export var events:Dictionary[String, PackedStringArray] = {}
