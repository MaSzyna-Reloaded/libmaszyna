@tool
extends EditorNode3DGizmoPlugin

## A sector's model is selected by a click on its bounds, the way the editor selects any node with
## a gizmo - an OPTIMIZED instance has no node of its own for the editor to find under the cursor

const ScenerySectorInspector = preload("./scenery_sector_inspector.gd")

var _sector_inspector:ScenerySectorInspector = null


func _init(sector_inspector:ScenerySectorInspector) -> void:
    _sector_inspector = sector_inspector


func _get_gizmo_name() -> String:
    return "SceneryModel"


func _has_gizmo(node:Node3D) -> bool:
    return node is E3DModelInstance and _sector_inspector.has_proxy(node)


func _redraw(gizmo:EditorNode3DGizmo) -> void:
    gizmo.clear()
    var bounds:AABB = (gizmo.get_node_3d() as E3DModelInstance).submodels_aabb
    # nothing to click before the model is streamed in
    if not bounds.has_volume():
        return
    var box := BoxMesh.new()
    box.size = bounds.size
    var faces:PackedVector3Array = box.get_faces()
    for i:int in faces.size():
        faces[i] += bounds.get_center()
    var triangles := TriangleMesh.new()
    triangles.create_from_faces(faces)
    gizmo.add_collision_triangles(triangles)
