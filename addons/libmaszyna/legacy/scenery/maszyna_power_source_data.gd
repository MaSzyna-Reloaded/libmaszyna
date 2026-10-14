@tool
extends Resource
class_name MaszynaPowerSourceData


@export var name:String = ""
@export var position:Vector3 = Vector3.ZERO
@export var nominal_voltage:float = 0.0
@export var voltage_frequency:float = 0.0
@export var internal_resistance:float = 0.0
@export var max_output_current:float = 0.0
@export var fast_fuse_timeout:float = 0.0
@export var fast_fuse_repetition:float = 0.0
@export var slow_fuse_timeout:float = 0.0
@export var is_section:bool = false
@export var recuperation:bool = false
