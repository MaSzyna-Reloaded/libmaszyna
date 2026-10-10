@tool
extends Resource
class_name TractionLine

@export var p1: Vector3 = Vector3.ZERO:
    set(value):
        if p1 == value:
            return
        p1 = value
        emit_changed()

@export var p2: Vector3 = Vector3.ZERO:
    set(value):
        if p2 == value:
            return
        p2 = value
        emit_changed()
