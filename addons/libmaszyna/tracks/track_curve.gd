@tool
extends Resource
class_name TrackCurve

@export var p1:Vector3 = Vector3.ZERO:
    set(x):
        if not p1 == x:
            p1 = x
            changed.emit()
@export var c1:Vector3 = Vector3.ZERO:
    set(x):
        if not c1 == x:
            c1 = x
            changed.emit()
@export var c2:Vector3 = Vector3.ZERO:
    set(x):
        if not c2 == x:
            c2 = x
            changed.emit()
@export var p2:Vector3 = Vector3.ZERO:
    set(x):
        if not p2 == x:
            p2 = x
            changed.emit()
@export var roll1:float = 0.0:
    set(x):
        if not roll1 == x:
            roll1 = x
            changed.emit()
@export var roll2:float = 0.0:
    set(x):
        if not roll2 == x:
            roll2 = x
            changed.emit()
@export var radius:float = 0.0:
    set(x):
        if not radius == x:
            radius = x
            changed.emit()


## A copy of the curve moved by `offset` - its handles are relative to its ends
func translated(offset:Vector3) -> TrackCurve:
    var moved:TrackCurve = duplicate() as TrackCurve
    moved.p1 += offset
    moved.p2 += offset
    return moved
