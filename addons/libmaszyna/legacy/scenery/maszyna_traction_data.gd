@tool
extends Resource
class_name MaszynaTractionData


@export var power_supply_name:String = ""
@export var nominal_voltage:float = 0.0
@export var max_current:float = 0.0
@export var resistivity:float = 0.0
@export var material:int = 0
@export var wire_thickness:float = 0.0
@export var damage_flag:int = 0
@export var contact_p1:Vector3 = Vector3.ZERO
@export var contact_p2:Vector3 = Vector3.ZERO
@export var support_p1:Vector3 = Vector3.ZERO
@export var support_p2:Vector3 = Vector3.ZERO
@export var min_height:float = 0.0
@export var segment_length:float = 0.0
@export var wires:int = TractionRenderingServer.Wires.CONTACT
@export var wire_offset:float = 0.0
@export var visible:bool = true
@export var parallel:String = ""
