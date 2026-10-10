@tool
extends Node3D
class_name Traction3D

const TRACTION_AL_MATERIAL = preload("res://addons/libmaszyna/legacy/materials/traction_al.tres")
const TRACTION_CU_MATERIAL = preload("res://addons/libmaszyna/legacy/materials/traction_cu.tres")
const TRACTION_AL_PATINA_MATERIAL = preload("res://addons/libmaszyna/legacy/materials/traction_al_patina.tres")
const TRACTION_CU_PATINA_MATERIAL = preload("res://addons/libmaszyna/legacy/materials/traction_cu_patina.tres")

enum DamageFlag {
    PATINA=1,
    BREAK=128
}

enum TractionMaterial {COPPER, ALUMINIUM}

func str_or_none(x):
    return str(x) if x else "none"

@export var wires:TractionRenderingServer.Wires:
    set(x):
        if not x == wires:
            _dirty = true
            wires = x

@export var contact_line: TractionLine:
    set(x):
        if not contact_line == x:
            _dirty = true
            if contact_line:
                contact_line.changed.disconnect(_on_line_changed)
            contact_line = x
            if contact_line:
                contact_line.changed.connect(_on_line_changed)

@export var support_line: TractionLine:
    set(x):
        if not support_line == x:
            _dirty = true
            if support_line:
                support_line.changed.disconnect(_on_line_changed)
            support_line = x
            if support_line:
                support_line.changed.connect(_on_line_changed)

@export var material:TractionMaterial:
    set(x):
        if not material == x:
            material = x
            _dirty = true

@export_flags("Patina:1", "Break:128") var damage_flag:int = 0:
    set(x):
        if not damage_flag == x:
            damage_flag = x
            _dirty = true

@export var power_supply_name:String
@export var nominal_voltage:float
@export var max_current:float
@export var resistivity:float
@export var wire_thickness:float  # mm
@export var min_height:float
@export var segment_length:float

@export var wire_offset:float
@export var parallel:String = ""

var _traction_rid: RID
var _dirty: bool = true


func _process_dirty(_delta: float) -> void:
    _update()


func _update():
    if not TractionRenderingServer or not _traction_rid or not _traction_rid.is_valid():
        return

    var contact_p1: Vector3 = contact_line.p1 if contact_line else Vector3.ZERO
    var contact_p2: Vector3 = contact_line.p2 if contact_line else Vector3.ZERO
    var support_p1: Vector3 = support_line.p1 if support_line else Vector3.ZERO
    var support_p2: Vector3 = support_line.p2 if support_line else Vector3.ZERO

    TractionRenderingServer.set_traction_geometry(
        _traction_rid,
        contact_p1,
        contact_p2,
        support_p1,
        support_p2,
        wire_thickness,
        wires,
        wire_offset,
        min_height,
        segment_length
    )
    TractionRenderingServer.set_traction_transform(_traction_rid, global_transform)
    TractionRenderingServer.set_traction_visible(_traction_rid, visible)
    TractionRenderingServer.set_traction_scenario(_traction_rid, get_world_3d().scenario)
    TractionRenderingServer.set_traction_material(_traction_rid, _get_material().get_rid())


func _on_line_changed() -> void:
    _dirty = true


func _enter_tree():
    if TractionRenderingServer:
        _traction_rid = TractionRenderingServer.create_traction()
    visibility_changed.connect(_update_visibility)


func _exit_tree():
    visibility_changed.disconnect(_update_visibility)

    if TractionRenderingServer and _traction_rid and _traction_rid.is_valid():
        TractionRenderingServer.free_traction(_traction_rid)
        _traction_rid = RID()


func _ready() -> void:
    set_process(true)
    set_notify_transform(true)
    _update()


func _notification(what):
    match what:
        NOTIFICATION_TRANSFORM_CHANGED, NOTIFICATION_VISIBILITY_CHANGED:
            if _traction_rid and TractionRenderingServer:
                TractionRenderingServer.set_traction_transform(_traction_rid, global_transform)
                TractionRenderingServer.set_traction_visible(_traction_rid, visible)


func _update_visibility():
    if _traction_rid and TractionRenderingServer:
        TractionRenderingServer.set_traction_visible(_traction_rid, visible)

func _process(delta: float) -> void:
    if Engine.is_editor_hint():
        if _dirty:
            _process_dirty(delta)
            _dirty = false

func _get_material():
    if damage_flag & DamageFlag.PATINA:
        return (
            TRACTION_CU_PATINA_MATERIAL if material == TractionMaterial.COPPER
            else TRACTION_AL_PATINA_MATERIAL
        )
    else:
        return (
            TRACTION_CU_MATERIAL if material == TractionMaterial.COPPER
            else TRACTION_AL_MATERIAL
        )
