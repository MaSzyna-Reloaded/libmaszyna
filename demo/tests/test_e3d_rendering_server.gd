extends MaszynaGutTest

## MaszynaE3DRenderingServer RID API: OPTIMIZED instances create no nodes, NODES instances build
## a node tree under the attached node and follow the lights state.
##
## The lights half covers what a scenery model node declares with `lights`/`lightcolors`: the
## server resolves those modes against the time of day and the light level, exactly as
## TAnimModel::RaPrepare() does (AnimModel.cpp:578-627).

## Full daylight and midday, the state a scenery starts in
const DAY_LIGHT_LEVEL: float = 1.0
const NIGHT_LIGHT_LEVEL: float = 0.1
const MIDDAY: float = 12.0
## 10:20:00 - what a clock shows of it (TSubModel::RaAnimation(), Model3d.cpp:1153-1211)
const CLOCK_TIME: float = 10.0 + 20.0 / 60.0
## A whole circle a minute, an hour, 12 hours or 24 hours; the minute hand on by a tenth of a degree
## a second
const SECOND_HAND_DEGREES_PER_SECOND: float = 6.0
const MINUTE_HAND_DEGREES_PER_SECOND: float = 0.1
const HOUR_HAND_DEGREES_PER_SECOND: float = 30.0 / 3600.0
const HOURS24_HAND_DEGREES_PER_SECOND: float = 15.0 / 3600.0
## By animation: the degrees its hand turns by a step of the simulation after CLOCK_TIME - the smooth
## ones a step further on
const CLOCK_HAND_DEGREES: Dictionary[MaszynaE3DSubModel.AnimationType, float] = {
    MaszynaE3DSubModel.ANIMATION_JUMP_SECONDS: 0.0,
    MaszynaE3DSubModel.ANIMATION_JUMP_MINUTES: 120.0,
    MaszynaE3DSubModel.ANIMATION_JUMP_HOURS: 300.0,
    MaszynaE3DSubModel.ANIMATION_JUMP_HOURS24: 150.0,
    MaszynaE3DSubModel.ANIMATION_SECONDS: SECOND_HAND_DEGREES_PER_SECOND * TICK,
    MaszynaE3DSubModel.ANIMATION_MINUTES: 120.0 + MINUTE_HAND_DEGREES_PER_SECOND * TICK,
    MaszynaE3DSubModel.ANIMATION_HOURS: 310.0 + HOUR_HAND_DEGREES_PER_SECOND * TICK,
    MaszynaE3DSubModel.ANIMATION_HOURS24: 155.0 + HOURS24_HAND_DEGREES_PER_SECOND * TICK,
}
## By drum (its digit): the degrees a digital clock's drum turns by at CLOCK_TIME, 36 backwards a
## digit - units and tens of the seconds, the minutes and the hours: 0 0, 0 2, 0 1
const CLOCK_DRUM_DEGREES: Dictionary[String, float] = {
    "0": 0.0, "1": 0.0, "2": 0.0, "3": -72.0, "4": 0.0, "5": -36.0,
}
## Single precision of the nodes' transforms
const CLOCK_TOLERANCE_DEGREES: float = 0.001


func after_each() -> void:
    # the time of day and the light level are singleton state shared by every test
    MaszynaE3DRenderingServer.environment_set_time(MIDDAY)
    MaszynaE3DRenderingServer.environment_set_light_level(DAY_LIGHT_LEVEL)


func test_optimized_instance_creates_no_nodes() -> void:
    var parent: Node3D = Node3D.new()
    add_child_autoqfree(parent)

    var rid: RID = MaszynaE3DRenderingServer.instance_create(build_lit_model(), ModelRenderingServer.INSTANCER_OPTIMIZED, ModelRenderingServer.INSTANCE_KIND_STATIC)
    MaszynaE3DRenderingServer.instance_attach_object_instance_id(rid, parent.get_instance_id())
    MaszynaE3DRenderingServer.instance_set_scenario(rid, parent.get_world_3d().scenario)
    MaszynaE3DRenderingServer.instance_set_transform(rid, Transform3D(Basis(), Vector3(10, 0, 0)))
    MaszynaE3DRenderingServer.instance_set_lights_state(rid, {"00": true})
    MaszynaE3DRenderingServer.instance_build(rid)

    assert_true(rid.is_valid())
    assert_eq(parent.get_child_count(true), 0)
    MaszynaE3DRenderingServer.instance_free(rid)


func test_nodes_instance_builds_tree_and_follows_lights_state() -> void:
    var parent: Node3D = Node3D.new()
    add_child_autoqfree(parent)

    var rid: RID = MaszynaE3DRenderingServer.instance_create(build_lit_model(), ModelRenderingServer.INSTANCER_NODES, ModelRenderingServer.INSTANCE_KIND_STATIC)
    MaszynaE3DRenderingServer.instance_attach_object_instance_id(rid, parent.get_instance_id())
    MaszynaE3DRenderingServer.instance_set_lights_state(rid, {"00": false})
    MaszynaE3DRenderingServer.instance_build(rid)

    var light_on: Node3D = parent.get_node(NodePath("light_on00"))
    var mesh: MeshInstance3D = parent.get_node(NodePath("light_on00/mesh"))
    assert_eq(parent.get_child_count(), 0, "generated nodes are internal")
    assert_eq(parent.get_child_count(true), 1)
    assert_not_null(mesh)
    assert_false(light_on.visible)

    MaszynaE3DRenderingServer.instance_set_lights_state(rid, {"00": true})
    assert_true(light_on.visible)

    MaszynaE3DRenderingServer.instance_free(rid)
    assert_eq(parent.get_child_count(true), 0)


func test_nodes_instance_rebuilds_on_options_change() -> void:
    var parent: Node3D = Node3D.new()
    add_child_autoqfree(parent)

    var rid: RID = MaszynaE3DRenderingServer.instance_create(build_lit_model(), ModelRenderingServer.INSTANCER_NODES, ModelRenderingServer.INSTANCE_KIND_STATIC)
    MaszynaE3DRenderingServer.instance_attach_object_instance_id(rid, parent.get_instance_id())
    MaszynaE3DRenderingServer.instance_build(rid)
    assert_not_null(parent.get_node_or_null(NodePath("light_on00/mesh")))

    MaszynaE3DRenderingServer.instance_set_options(rid, "", [], ["mesh"], false, [], 0)
    assert_eq(parent.get_child_count(true), 1)
    assert_null(parent.get_node_or_null(NodePath("light_on00/mesh")))
    MaszynaE3DRenderingServer.instance_free(rid)


## opengl33renderer.cpp:1208 - the pick pass draws no translucent submodel (E186's glass caps over
## the spring brake buttons)
func test_opaque_meshes_leave_out_translucent_submodels() -> void:
    var parent: Node3D = Node3D.new()
    add_child_autoqfree(parent)
    var model: MaszynaE3DModel = build_lit_model()
    var mesh_submodel: MaszynaE3DSubModel = model.submodels[0].submodels[0]
    var cap: MaszynaE3DSubModel = MaszynaE3DSubModel.new()
    cap.resource_name = "cap"
    cap.submodel_type = MaszynaE3DSubModel.SUBMODEL_GL_TRIANGLES
    cap.mesh = mesh_submodel.mesh
    cap.material_transparent = true
    var mesh_children: Array[MaszynaE3DSubModel] = [cap]
    mesh_submodel.submodels = mesh_children

    var rid: RID = MaszynaE3DRenderingServer.instance_create(model, ModelRenderingServer.INSTANCER_NODES, ModelRenderingServer.INSTANCE_KIND_STATIC)
    MaszynaE3DRenderingServer.instance_attach_object_instance_id(rid, parent.get_instance_id())
    MaszynaE3DRenderingServer.instance_build(rid)

    var mesh: MeshInstance3D = parent.get_node(NodePath("light_on00/mesh"))
    var expected: PackedInt64Array = [mesh.get_instance_id()]
    assert_eq(MaszynaE3DRenderingServer.instance_get_opaque_meshes(rid), expected)
    MaszynaE3DRenderingServer.instance_free(rid)


## A vehicle's glass - a translucent submodel of an instance that forces alpha - is blended while
## drawn as nodes and opaque in the optimized instancer, which never uses the alpha pass; the
## original draws it in its alpha pass (opengl33renderer.cpp:4313). The rest stays as it is.
func test_forced_translucent_submodel_is_blended_as_nodes_and_opaque_optimized() -> void:
    var parent: Node3D = Node3D.new()
    add_child_autoqfree(parent)
    var model: MaszynaE3DModel = build_lit_model()
    var mesh_submodel: MaszynaE3DSubModel = model.submodels[0].submodels[0]
    mesh_submodel.material_name = "body"
    var glass: MaszynaE3DSubModel = MaszynaE3DSubModel.new()
    glass.resource_name = "glass"
    glass.submodel_type = MaszynaE3DSubModel.SUBMODEL_GL_TRIANGLES
    glass.mesh = mesh_submodel.mesh
    glass.material_name = "glass"
    glass.skin_translucent = true
    var mesh_children: Array[MaszynaE3DSubModel] = [glass]
    mesh_submodel.submodels = mesh_children
    var resolved: Dictionary[String, int] = {}
    MaszynaE3DRenderingServer.material_set_resolver(
        func(submodel: MaszynaE3DSubModel, _data_path: String, _skins: PackedStringArray,
                translucency: MaszynaE3DRenderingServer.Translucency, _max_texture_size: int) -> Material:
            resolved[submodel.resource_name] = translucency
            return null)

    for instancer: ModelRenderingServer.Instancer in [ModelRenderingServer.INSTANCER_NODES, ModelRenderingServer.INSTANCER_OPTIMIZED]:
        resolved.clear()
        var rid: RID = MaszynaE3DRenderingServer.instance_create(model, instancer, ModelRenderingServer.INSTANCE_KIND_DYNAMIC)
        MaszynaE3DRenderingServer.instance_set_options(rid, "", [], [], true, [], 0)
        MaszynaE3DRenderingServer.instance_attach_object_instance_id(rid, parent.get_instance_id())
        MaszynaE3DRenderingServer.instance_set_scenario(rid, parent.get_world_3d().scenario)
        MaszynaE3DRenderingServer.instance_build(rid)
        var forced: MaszynaE3DRenderingServer.Translucency = (
                MaszynaE3DRenderingServer.TRANSLUCENCY_BLENDED if instancer == ModelRenderingServer.INSTANCER_NODES
                else MaszynaE3DRenderingServer.TRANSLUCENCY_OPAQUE)
        assert_eq(resolved.get("glass", -1), forced, "the glass, instancer %d" % instancer)
        assert_eq(resolved.get("mesh", -1), MaszynaE3DRenderingServer.TRANSLUCENCY_CUTOUT, "the body, instancer %d" % instancer)
        MaszynaE3DRenderingServer.instance_free(rid)

    MaszynaE3DRenderingServer.material_set_resolver(MaszynaMaterialManager.get_submodel_material)


## A submodel's visibility range is measured from the model's origin, as the original measures one
## distance for every submodel (opengl33renderer.cpp:3388, 3654); Godot measures it to the centre
## of the box, which is therefore centred there - two LODs of one part with meshes of their own
## centres left a gap around their common bound (34WE's body at 80 m).
func test_submodel_box_is_centred_on_the_model_origin() -> void:
    var parent: Node3D = Node3D.new()
    add_child_autoqfree(parent)
    var model: MaszynaE3DModel = build_lit_model()
    var offset: Vector3 = Vector3(0.0, 2.0, 5.0)
    model.submodels[0].transform = Transform3D(Basis(), offset)
    model.submodels[0].submodels[0].transform = Transform3D(Basis(), offset)

    var rid: RID = MaszynaE3DRenderingServer.instance_create(model, ModelRenderingServer.INSTANCER_NODES, ModelRenderingServer.INSTANCE_KIND_STATIC)
    MaszynaE3DRenderingServer.instance_attach_object_instance_id(rid, parent.get_instance_id())
    MaszynaE3DRenderingServer.instance_build(rid)

    var mesh: MeshInstance3D = parent.get_node(NodePath("light_on00/mesh"))
    assert_almost_eq(mesh.transform * mesh.custom_aabb.get_center() + offset, Vector3.ZERO, Vector3.ONE * 0.001)
    assert_true(mesh.custom_aabb.encloses(mesh.get_aabb()), "the box still holds the mesh")
    MaszynaE3DRenderingServer.instance_free(rid)


func test_dark_light_follows_the_light_level() -> void:
    var parent: Node3D = Node3D.new()
    add_child_autoqfree(parent)
    var rid: RID = build_lit_instance(parent)
    # `lights 3` - the mode every street lamp in the data set declares
    MaszynaE3DRenderingServer.instance_set_lights_modes(rid, [float(MaszynaE3DRenderingServer.LIGHT_MODE_DARK)])
    var light_on: Node3D = parent.get_node(NodePath("light_on00"))

    MaszynaE3DRenderingServer.environment_set_light_level(DAY_LIGHT_LEVEL)
    assert_false(light_on.visible, "unlit in daylight")

    MaszynaE3DRenderingServer.environment_set_light_level(NIGHT_LIGHT_LEVEL)
    assert_true(light_on.visible, "lit once it gets dark")

    MaszynaE3DRenderingServer.instance_free(rid)


func test_dark_light_fraction_is_its_own_threshold() -> void:
    var parent: Node3D = Node3D.new()
    add_child_autoqfree(parent)
    var rid: RID = build_lit_instance(parent)
    # `lights 3.4` - comes on below a light level of 0.4 instead of the default 0.325
    MaszynaE3DRenderingServer.instance_set_lights_modes(rid, [3.4])
    var light_on: Node3D = parent.get_node(NodePath("light_on00"))

    MaszynaE3DRenderingServer.environment_set_light_level(0.5)
    assert_false(light_on.visible, "still above its own threshold")

    MaszynaE3DRenderingServer.environment_set_light_level(0.35)
    assert_true(light_on.visible, "below its own threshold, but above the default one")

    MaszynaE3DRenderingServer.instance_free(rid)


func test_home_light_is_forced_off_late_at_night() -> void:
    var parent: Node3D = Node3D.new()
    add_child_autoqfree(parent)
    var rid: RID = build_lit_instance(parent)
    MaszynaE3DRenderingServer.instance_set_lights_modes(rid, [float(MaszynaE3DRenderingServer.LIGHT_MODE_HOME)])
    var light_on: Node3D = parent.get_node(NodePath("light_on00"))

    MaszynaE3DRenderingServer.environment_set_light_level(NIGHT_LIGHT_LEVEL)
    MaszynaE3DRenderingServer.environment_set_time(22.0)
    assert_true(light_on.visible, "a lit window in the evening")

    MaszynaE3DRenderingServer.environment_set_time(3.0)
    assert_false(light_on.visible, "the same window is dark between 1:00 and 5:00")

    MaszynaE3DRenderingServer.environment_set_time(6.0)
    assert_true(light_on.visible, "and lit again before dawn")

    MaszynaE3DRenderingServer.instance_free(rid)


func test_lights_state_overrides_the_declared_mode() -> void:
    var parent: Node3D = Node3D.new()
    add_child_autoqfree(parent)
    var rid: RID = build_lit_instance(parent)
    MaszynaE3DRenderingServer.instance_set_lights_modes(rid, [float(MaszynaE3DRenderingServer.LIGHT_MODE_OFF)])
    var light_on: Node3D = parent.get_node(NodePath("light_on00"))
    assert_false(light_on.visible)

    MaszynaE3DRenderingServer.instance_set_lights_state(rid, {"00": true})
    assert_true(light_on.visible, "a manual state wins over the declared mode")

    MaszynaE3DRenderingServer.instance_free(rid)


func test_emission_light_handle_switches_the_submodels() -> void:
    var parent: Node3D = Node3D.new()
    add_child_autoqfree(parent)
    var rid: RID = build_lit_instance(parent)
    var light_on: Node3D = parent.get_node(NodePath("light_on00"))

    var light: RID = MaszynaE3DRenderingServer.emission_light_create(rid, "00")
    assert_true(light.is_valid())

    MaszynaE3DRenderingServer.light_enable(light)
    assert_true(light_on.visible)

    MaszynaE3DRenderingServer.light_disable(light)
    assert_false(light_on.visible)

    MaszynaE3DRenderingServer.light_free(light)
    MaszynaE3DRenderingServer.instance_free(rid)


func test_instance_free_releases_its_lights() -> void:
    var parent: Node3D = Node3D.new()
    add_child_autoqfree(parent)
    var rid: RID = build_lit_instance(parent)
    var before: int = MaszynaE3DRenderingServer.light_get_statistics()["total"]

    MaszynaE3DRenderingServer.emission_light_create(rid, "00")
    assert_eq(MaszynaE3DRenderingServer.light_get_statistics()["total"], before + 1)

    MaszynaE3DRenderingServer.instance_free(rid)
    assert_eq(
        MaszynaE3DRenderingServer.light_get_statistics()["total"], before, "freeing the instance frees its lights"
    )


func test_the_time_turns_the_hands_and_drums_of_a_clock() -> void:
    var previous_time:float = SimulationServer.time_of_day
    var model: MaszynaE3DModel = MaszynaE3DModel.new()
    var hands: Array[MaszynaE3DSubModel] = []
    for animation: MaszynaE3DSubModel.AnimationType in CLOCK_HAND_DEGREES:
        var hand: MaszynaE3DSubModel = MaszynaE3DSubModel.new()
        hand.resource_name = "hand%d" % animation
        hand.submodel_type = MaszynaE3DSubModel.SUBMODEL_TRANSFORM
        hand.animation = animation
        hands.append(hand)
    var digital_clock: MaszynaE3DSubModel = MaszynaE3DSubModel.new()
    digital_clock.resource_name = "digital"
    digital_clock.submodel_type = MaszynaE3DSubModel.SUBMODEL_TRANSFORM
    digital_clock.animation = MaszynaE3DSubModel.ANIMATION_DIGICLK
    var drums: Array[MaszynaE3DSubModel] = []
    for drum_name: String in CLOCK_DRUM_DEGREES:
        var drum: MaszynaE3DSubModel = MaszynaE3DSubModel.new()
        drum.resource_name = drum_name
        drum.submodel_type = MaszynaE3DSubModel.SUBMODEL_TRANSFORM
        drums.append(drum)
    digital_clock.submodels = drums
    hands.append(digital_clock)
    model.submodels = hands
    var parent: Node3D = Node3D.new()
    add_child_autoqfree(parent)
    var rid: RID = MaszynaE3DRenderingServer.instance_create(model, ModelRenderingServer.INSTANCER_NODES, ModelRenderingServer.INSTANCE_KIND_STATIC)
    MaszynaE3DRenderingServer.instance_attach_object_instance_id(rid, parent.get_instance_id())
    MaszynaE3DRenderingServer.instance_build(rid)

    SimulationServer.time_of_day = CLOCK_TIME
    await step(1)

    for animation: MaszynaE3DSubModel.AnimationType in CLOCK_HAND_DEGREES:
        var hand_node: Node3D = parent.get_node(NodePath("hand%d" % animation))
        assert_almost_eq(_turn_about_y(hand_node), wrapf(CLOCK_HAND_DEGREES[animation], -180.0, 180.0), CLOCK_TOLERANCE_DEGREES,
                "hand of animation %d" % animation)
    for drum_name: String in CLOCK_DRUM_DEGREES:
        var drum_node: Node3D = parent.get_node(NodePath("digital/" + drum_name))
        assert_almost_eq(_turn_about_y(drum_node), CLOCK_DRUM_DEGREES[drum_name], CLOCK_TOLERANCE_DEGREES, "drum " + drum_name)
    MaszynaE3DRenderingServer.instance_free(rid)
    SimulationServer.time_of_day = previous_time


## The degrees a node is turned by about its y
func _turn_about_y(node: Node3D) -> float:
    return rad_to_deg(atan2(-node.basis.x.z, node.basis.x.x))
