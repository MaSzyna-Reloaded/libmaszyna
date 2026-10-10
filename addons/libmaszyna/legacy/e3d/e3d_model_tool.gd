@tool
extends Node


## Returns the first mesh from the E3D submodel tree using depth-first preorder.
func find_first_mesh(model: E3DModel) -> ArrayMesh:
    if not model:
        return null

    return _find_first_mesh_in_submodels(model.submodels)


## Compute AABB for E3D model
func get_aabb(model: E3DModel) -> AABB:
    if not model:
        return AABB()

    var result = null
    for item: Variant in model.submodels:
        if item is E3DSubModel:
            var submodel_aabb = _get_aabb_from_submodel(item as E3DSubModel, Transform3D.IDENTITY)
            if submodel_aabb:
                result = result.merge(submodel_aabb) if result else submodel_aabb
    return result if result else AABB()


func _get_aabb_from_submodel(submodel: E3DSubModel, parent_transform: Transform3D):
    var local_transform: Transform3D = parent_transform * submodel.transform
    var result = local_transform * submodel.mesh.get_aabb() if submodel.mesh else null

    for inner_submodel: E3DSubModel in submodel.submodels:
        var inner_aabb = _get_aabb_from_submodel(inner_submodel, local_transform)
        if inner_aabb:
            result = result.merge(inner_aabb) if result else inner_aabb
    return result


## Returns null when the current branch does not contain any mesh.
func _find_first_mesh_in_submodels(submodels: Array) -> ArrayMesh:
    for item: Variant in submodels:
        if not item is E3DSubModel:
            continue

        var submodel: E3DSubModel = item as E3DSubModel
        if submodel.mesh:
            return submodel.mesh

        var mesh: ArrayMesh = _find_first_mesh_in_submodels(submodel.submodels)
        if mesh:
            return mesh

    return null
