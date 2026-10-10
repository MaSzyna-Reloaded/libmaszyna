extends MaszynaGutTest

## E3DRenderingServer RID API: OPTIMIZED instances create no nodes, NODES instances build
## a node tree under the attached node and follow the lights state.
##
## The lights half covers what a scenery model node declares with `lights`/`lightcolors`: the
## server resolves those modes against the time of day and the light level, exactly as
## TAnimModel::RaPrepare() does (AnimModel.cpp:578-627).

## Full daylight and midday, the state a scenery starts in
const DAY_LIGHT_LEVEL: float = 1.0
const NIGHT_LIGHT_LEVEL: float = 0.1
const MIDDAY: float = 12.0


func after_each() -> void:
    # the time of day and the light level are singleton state shared by every test
    E3DRenderingServer.environment_set_time(MIDDAY)
    E3DRenderingServer.environment_set_light_level(DAY_LIGHT_LEVEL)


func test_optimized_instance_creates_no_nodes() -> void:
    var parent: Node3D = Node3D.new()
    add_child_autoqfree(parent)

    var rid: RID = E3DRenderingServer.instance_create(build_lit_model(), E3DRenderingServer.INSTANCER_OPTIMIZED, E3DRenderingServer.INSTANCE_KIND_STATIC)
    E3DRenderingServer.instance_attach_object_instance_id(rid, parent.get_instance_id())
    E3DRenderingServer.instance_set_scenario(rid, parent.get_world_3d().scenario)
    E3DRenderingServer.instance_set_transform(rid, Transform3D(Basis(), Vector3(10, 0, 0)))
    E3DRenderingServer.instance_set_lights_state(rid, {"00": true})
    E3DRenderingServer.instance_build(rid)

    assert_true(rid.is_valid())
    assert_eq(parent.get_child_count(true), 0)
    E3DRenderingServer.instance_free(rid)


func test_nodes_instance_builds_tree_and_follows_lights_state() -> void:
    var parent: Node3D = Node3D.new()
    add_child_autoqfree(parent)

    var rid: RID = E3DRenderingServer.instance_create(build_lit_model(), E3DRenderingServer.INSTANCER_NODES, E3DRenderingServer.INSTANCE_KIND_STATIC)
    E3DRenderingServer.instance_attach_object_instance_id(rid, parent.get_instance_id())
    E3DRenderingServer.instance_set_lights_state(rid, {"00": false})
    E3DRenderingServer.instance_build(rid)

    var light_on: Node3D = parent.get_node(NodePath("light_on00"))
    var mesh: MeshInstance3D = parent.get_node(NodePath("light_on00/mesh"))
    assert_eq(parent.get_child_count(), 0, "generated nodes are internal")
    assert_eq(parent.get_child_count(true), 1)
    assert_not_null(mesh)
    assert_false(light_on.visible)

    E3DRenderingServer.instance_set_lights_state(rid, {"00": true})
    assert_true(light_on.visible)

    E3DRenderingServer.instance_free(rid)
    assert_eq(parent.get_child_count(true), 0)


func test_nodes_instance_rebuilds_on_options_change() -> void:
    var parent: Node3D = Node3D.new()
    add_child_autoqfree(parent)

    var rid: RID = E3DRenderingServer.instance_create(build_lit_model(), E3DRenderingServer.INSTANCER_NODES, E3DRenderingServer.INSTANCE_KIND_STATIC)
    E3DRenderingServer.instance_attach_object_instance_id(rid, parent.get_instance_id())
    E3DRenderingServer.instance_build(rid)
    assert_not_null(parent.get_node_or_null(NodePath("light_on00/mesh")))

    E3DRenderingServer.instance_set_options(rid, "", [], ["mesh"], false, [], 0)
    assert_eq(parent.get_child_count(true), 1)
    assert_null(parent.get_node_or_null(NodePath("light_on00/mesh")))
    E3DRenderingServer.instance_free(rid)


## opengl33renderer.cpp:1208 - the pick pass draws no translucent submodel (E186's glass caps over
## the spring brake buttons)
func test_opaque_meshes_leave_out_translucent_submodels() -> void:
    var parent: Node3D = Node3D.new()
    add_child_autoqfree(parent)
    var model: E3DModel = build_lit_model()
    var mesh_submodel: E3DSubModel = model.submodels[0].submodels[0]
    var cap: E3DSubModel = E3DSubModel.new()
    cap.resource_name = "cap"
    cap.submodel_type = E3DSubModel.SUBMODEL_GL_TRIANGLES
    cap.mesh = mesh_submodel.mesh
    cap.material_transparent = true
    var mesh_children: Array[E3DSubModel] = [cap]
    mesh_submodel.submodels = mesh_children

    var rid: RID = E3DRenderingServer.instance_create(model, E3DRenderingServer.INSTANCER_NODES, E3DRenderingServer.INSTANCE_KIND_STATIC)
    E3DRenderingServer.instance_attach_object_instance_id(rid, parent.get_instance_id())
    E3DRenderingServer.instance_build(rid)

    var mesh: MeshInstance3D = parent.get_node(NodePath("light_on00/mesh"))
    var expected: PackedInt64Array = [mesh.get_instance_id()]
    assert_eq(E3DRenderingServer.instance_get_opaque_meshes(rid), expected)
    E3DRenderingServer.instance_free(rid)


## A vehicle's glass - a translucent submodel of an instance that forces alpha - is blended while
## drawn as nodes and opaque in the optimized instancer, which never uses the alpha pass; the
## original draws it in its alpha pass (opengl33renderer.cpp:4313). The rest stays as it is.
func test_forced_translucent_submodel_is_blended_as_nodes_and_opaque_optimized() -> void:
    var parent: Node3D = Node3D.new()
    add_child_autoqfree(parent)
    var model: E3DModel = build_lit_model()
    var mesh_submodel: E3DSubModel = model.submodels[0].submodels[0]
    mesh_submodel.material_name = "body"
    var glass: E3DSubModel = E3DSubModel.new()
    glass.resource_name = "glass"
    glass.submodel_type = E3DSubModel.SUBMODEL_GL_TRIANGLES
    glass.mesh = mesh_submodel.mesh
    glass.material_name = "glass"
    glass.skin_translucent = true
    var mesh_children: Array[E3DSubModel] = [glass]
    mesh_submodel.submodels = mesh_children
    var resolved: Dictionary[String, int] = {}
    E3DRenderingServer.material_set_resolver(
        func(submodel: E3DSubModel, _data_path: String, _skins: PackedStringArray,
                translucency: E3DRenderingServer.Translucency, _max_texture_size: int) -> Material:
            resolved[submodel.resource_name] = translucency
            return null)

    for instancer: E3DRenderingServer.Instancer in [E3DRenderingServer.INSTANCER_NODES, E3DRenderingServer.INSTANCER_OPTIMIZED]:
        resolved.clear()
        var rid: RID = E3DRenderingServer.instance_create(model, instancer, E3DRenderingServer.INSTANCE_KIND_DYNAMIC)
        E3DRenderingServer.instance_set_options(rid, "", [], [], true, [], 0)
        E3DRenderingServer.instance_attach_object_instance_id(rid, parent.get_instance_id())
        E3DRenderingServer.instance_set_scenario(rid, parent.get_world_3d().scenario)
        E3DRenderingServer.instance_build(rid)
        var forced: E3DRenderingServer.Translucency = (
                E3DRenderingServer.TRANSLUCENCY_BLENDED if instancer == E3DRenderingServer.INSTANCER_NODES
                else E3DRenderingServer.TRANSLUCENCY_OPAQUE)
        assert_eq(resolved.get("glass", -1), forced, "the glass, instancer %d" % instancer)
        assert_eq(resolved.get("mesh", -1), E3DRenderingServer.TRANSLUCENCY_CUTOUT, "the body, instancer %d" % instancer)
        E3DRenderingServer.instance_free(rid)

    E3DRenderingServer.material_set_resolver(MaterialManager.get_submodel_material)


## A submodel's visibility range is measured from the model's origin, as the original measures one
## distance for every submodel (opengl33renderer.cpp:3388, 3654); Godot measures it to the centre
## of the box, which is therefore centred there - two LODs of one part with meshes of their own
## centres left a gap around their common bound (34WE's body at 80 m).
func test_submodel_box_is_centred_on_the_model_origin() -> void:
    var parent: Node3D = Node3D.new()
    add_child_autoqfree(parent)
    var model: E3DModel = build_lit_model()
    var offset: Vector3 = Vector3(0.0, 2.0, 5.0)
    model.submodels[0].transform = Transform3D(Basis(), offset)
    model.submodels[0].submodels[0].transform = Transform3D(Basis(), offset)

    var rid: RID = E3DRenderingServer.instance_create(model, E3DRenderingServer.INSTANCER_NODES, E3DRenderingServer.INSTANCE_KIND_STATIC)
    E3DRenderingServer.instance_attach_object_instance_id(rid, parent.get_instance_id())
    E3DRenderingServer.instance_build(rid)

    var mesh: MeshInstance3D = parent.get_node(NodePath("light_on00/mesh"))
    assert_almost_eq(mesh.transform * mesh.custom_aabb.get_center() + offset, Vector3.ZERO, Vector3.ONE * 0.001)
    assert_true(mesh.custom_aabb.encloses(mesh.get_aabb()), "the box still holds the mesh")
    E3DRenderingServer.instance_free(rid)


func test_dark_light_follows_the_light_level() -> void:
    var parent: Node3D = Node3D.new()
    add_child_autoqfree(parent)
    var rid: RID = build_lit_instance(parent)
    # `lights 3` - the mode every street lamp in the data set declares
    E3DRenderingServer.instance_set_lights_modes(rid, [float(E3DRenderingServer.LIGHT_MODE_DARK)])
    var light_on: Node3D = parent.get_node(NodePath("light_on00"))

    E3DRenderingServer.environment_set_light_level(DAY_LIGHT_LEVEL)
    assert_false(light_on.visible, "unlit in daylight")

    E3DRenderingServer.environment_set_light_level(NIGHT_LIGHT_LEVEL)
    assert_true(light_on.visible, "lit once it gets dark")

    E3DRenderingServer.instance_free(rid)


func test_dark_light_fraction_is_its_own_threshold() -> void:
    var parent: Node3D = Node3D.new()
    add_child_autoqfree(parent)
    var rid: RID = build_lit_instance(parent)
    # `lights 3.4` - comes on below a light level of 0.4 instead of the default 0.325
    E3DRenderingServer.instance_set_lights_modes(rid, [3.4])
    var light_on: Node3D = parent.get_node(NodePath("light_on00"))

    E3DRenderingServer.environment_set_light_level(0.5)
    assert_false(light_on.visible, "still above its own threshold")

    E3DRenderingServer.environment_set_light_level(0.35)
    assert_true(light_on.visible, "below its own threshold, but above the default one")

    E3DRenderingServer.instance_free(rid)


func test_home_light_is_forced_off_late_at_night() -> void:
    var parent: Node3D = Node3D.new()
    add_child_autoqfree(parent)
    var rid: RID = build_lit_instance(parent)
    E3DRenderingServer.instance_set_lights_modes(rid, [float(E3DRenderingServer.LIGHT_MODE_HOME)])
    var light_on: Node3D = parent.get_node(NodePath("light_on00"))

    E3DRenderingServer.environment_set_light_level(NIGHT_LIGHT_LEVEL)
    E3DRenderingServer.environment_set_time(22.0)
    assert_true(light_on.visible, "a lit window in the evening")

    E3DRenderingServer.environment_set_time(3.0)
    assert_false(light_on.visible, "the same window is dark between 1:00 and 5:00")

    E3DRenderingServer.environment_set_time(6.0)
    assert_true(light_on.visible, "and lit again before dawn")

    E3DRenderingServer.instance_free(rid)


func test_lights_state_overrides_the_declared_mode() -> void:
    var parent: Node3D = Node3D.new()
    add_child_autoqfree(parent)
    var rid: RID = build_lit_instance(parent)
    E3DRenderingServer.instance_set_lights_modes(rid, [float(E3DRenderingServer.LIGHT_MODE_OFF)])
    var light_on: Node3D = parent.get_node(NodePath("light_on00"))
    assert_false(light_on.visible)

    E3DRenderingServer.instance_set_lights_state(rid, {"00": true})
    assert_true(light_on.visible, "a manual state wins over the declared mode")

    E3DRenderingServer.instance_free(rid)


func test_emission_light_handle_switches_the_submodels() -> void:
    var parent: Node3D = Node3D.new()
    add_child_autoqfree(parent)
    var rid: RID = build_lit_instance(parent)
    var light_on: Node3D = parent.get_node(NodePath("light_on00"))

    var light: RID = E3DRenderingServer.emission_light_create(rid, "00")
    assert_true(light.is_valid())

    E3DRenderingServer.light_enable(light)
    assert_true(light_on.visible)

    E3DRenderingServer.light_disable(light)
    assert_false(light_on.visible)

    E3DRenderingServer.light_free(light)
    E3DRenderingServer.instance_free(rid)


func test_instance_free_releases_its_lights() -> void:
    var parent: Node3D = Node3D.new()
    add_child_autoqfree(parent)
    var rid: RID = build_lit_instance(parent)
    var before: int = E3DRenderingServer.light_get_statistics()["total"]

    E3DRenderingServer.emission_light_create(rid, "00")
    assert_eq(E3DRenderingServer.light_get_statistics()["total"], before + 1)

    E3DRenderingServer.instance_free(rid)
    assert_eq(
        E3DRenderingServer.light_get_statistics()["total"], before, "freeing the instance frees its lights"
    )
