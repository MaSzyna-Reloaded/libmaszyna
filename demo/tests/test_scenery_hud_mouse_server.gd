extends MaszynaGutTest

## SceneryHUDMouseServer: a scenery model is hovered through the camera ray, a click calls its
## operation, Shift+click the other one (basic_cell::on_click(), scene.cpp:33-43, 691-704); and
## E3DRenderingServer's ray test the picking stands on.

const EventImporter = preload("res://addons/libmaszyna/legacy/scenery/maszyna_event_importer.gd")
const NodeImporter = preload("res://addons/libmaszyna/legacy/scenery/maszyna_node_importer.gd")
## Longer than any test runs, so a fired event stays queued
const NEVER:float = 3600.0
## Where the model stands in front of the camera
const MODEL_POSITION:Vector3 = Vector3(0.0, 0.0, -10.0)

var _camera:Camera3D
var _presses:Array[StringName] = []


func before_each() -> void:
    _presses.clear()
    _camera = Camera3D.new()
    add_child_autoqfree(_camera)
    _camera.make_current()
    SceneryHUDMouseServer.mouse_set_camera(_camera.get_instance_id())


func after_each() -> void:
    SceneryHUDMouseServer.mouse_set_active(true)


func test_the_segment_hits_a_built_instance_and_misses_beside_it() -> void:
    var instance:RID = _create_instance()

    var hit:Dictionary = E3DRenderingServer.instance_intersect_segment(instance, Vector3.ZERO, Vector3(0.0, 0.0, -100.0))
    var miss:Dictionary = E3DRenderingServer.instance_intersect_segment(instance, Vector3(5.0, 0.0, 0.0), Vector3(5.0, 0.0, -100.0))

    # the box is 1 m wide, its near face half a metre in front of its middle
    assert_almost_eq(float(hit["distance"]), 9.5, 0.001)
    assert_true((hit["position"] as Vector3).is_equal_approx(Vector3(0.0, 0.0, -9.5)))
    assert_true(miss.is_empty())
    E3DRenderingServer.instance_free(instance)


func test_an_instance_not_built_is_not_hit() -> void:
    var instance:RID = E3DRenderingServer.instance_create(_create_model(), E3DRenderingServer.INSTANCER_OPTIMIZED, E3DRenderingServer.INSTANCE_KIND_STATIC)

    assert_true(E3DRenderingServer.instance_intersect_segment(instance, Vector3.ZERO, Vector3(0.0, 0.0, -100.0)).is_empty())
    E3DRenderingServer.instance_free(instance)


func test_a_click_calls_pressed_and_a_shift_click_shift_pressed() -> void:
    var instance:RID = _create_instance()
    var pickable:RID = SceneryHUDMouseServer.pickable_create(instance, "lever", "", _press.bind(&"pressed"), _press.bind(&"shift_pressed"))

    SceneryHUDMouseServer.mouse_input(_motion(_camera.unproject_position(MODEL_POSITION)))
    assert_eq(SceneryHUDMouseServer.pickable_get_hovered(), pickable)
    assert_true(SceneryHUDMouseServer.mouse_input(_click(false)))
    assert_true(SceneryHUDMouseServer.mouse_input(_click(true)))

    var expected:Array[StringName] = [&"pressed", &"shift_pressed"]
    assert_eq(_presses, expected)
    SceneryHUDMouseServer.pickable_free(pickable)
    E3DRenderingServer.instance_free(instance)


func test_nothing_is_hovered_off_the_model_or_while_inactive() -> void:
    var instance:RID = _create_instance()
    var pickable:RID = SceneryHUDMouseServer.pickable_create(instance, "lever", "", _press.bind(&"pressed"), _press.bind(&"shift_pressed"))

    SceneryHUDMouseServer.mouse_input(_motion(_camera.unproject_position(MODEL_POSITION + Vector3(5.0, 0.0, 0.0))))
    assert_false(SceneryHUDMouseServer.pickable_get_hovered().is_valid(), "beside the model")
    assert_false(SceneryHUDMouseServer.mouse_input(_click(false)))

    SceneryHUDMouseServer.mouse_input(_motion(_camera.unproject_position(MODEL_POSITION)))
    SceneryHUDMouseServer.mouse_set_active(false)
    assert_false(SceneryHUDMouseServer.pickable_get_hovered().is_valid(), "made inactive")
    assert_false(SceneryHUDMouseServer.mouse_input(_click(false)))

    assert_true(_presses.is_empty())
    SceneryHUDMouseServer.pickable_free(pickable)
    E3DRenderingServer.instance_free(instance)


func test_a_click_on_a_vehicle_model_announces_the_vehicle() -> void:
    var instance:RID = _create_instance()
    var vehicle:RID = VehicleServer.vehicle_create()
    var pickable:RID = SceneryHUDMouseServer.vehicle_pickable_create(instance, "EU07-424", vehicle)
    watch_signals(SceneryHUDMouseServer)

    SceneryHUDMouseServer.mouse_input(_motion(_camera.unproject_position(MODEL_POSITION)))
    assert_signal_emitted_with_parameters(SceneryHUDMouseServer, "pickable_hovered", ["EU07-424", ""])
    assert_true(SceneryHUDMouseServer.mouse_input(_click(false)))
    assert_signal_emitted_with_parameters(SceneryHUDMouseServer, "vehicle_pressed", [vehicle])

    SceneryHUDMouseServer.pickable_free(pickable)
    E3DRenderingServer.instance_free(instance)
    VehicleServer.vehicle_free(vehicle)


func test_a_pickable_goes_with_its_instance() -> void:
    var instance:RID = _create_instance()
    var vehicle:RID = VehicleServer.vehicle_create()
    SceneryHUDMouseServer.vehicle_pickable_create(instance, "EU07-424", vehicle)
    SceneryHUDMouseServer.mouse_input(_motion(_camera.unproject_position(MODEL_POSITION)))
    watch_signals(SceneryHUDMouseServer)

    E3DRenderingServer.instance_free(instance)

    assert_false(SceneryHUDMouseServer.pickable_get_hovered().is_valid())
    assert_signal_emitted(SceneryHUDMouseServer, "pickable_unhovered")
    assert_false(SceneryHUDMouseServer.mouse_input(_click(false)))
    VehicleServer.vehicle_free(vehicle)


func test_a_scenery_model_fires_the_launcher_of_its_name_that_has_it_in_range() -> void:
    var lever:RID = _create_instance()
    var far_lever:RID = _create_instance()
    var beside:Vector3 = MODEL_POSITION + Vector3(5.0, 0.0, 0.0)
    E3DRenderingServer.instance_set_transform(far_lever, Transform3D(Basis(), beside))
    var models:Array[MaszynaModelData] = [_model_data("zwr1", MODEL_POSITION), _model_data("zwr2", beside)]
    var model_rids:Array[RID] = [lever, far_lever]
    # zwr1 is within 20 m of its launcher; zwr2's launcher has a negative radius, which the
    # original never finds a model within (EvLaunch.cpp:57-58, scene.cpp:37)
    var context:MaszynaImporterContext = _parse(
        "node -1 0 zwr1 eventlauncher 0 0 0 20 k 0 zwr1+ zwr1- end "
        + "node -1 0 zwr2 eventlauncher 0 0 0 -1 none 0 zwr2+ zwr2- end "
        + "event zwr1+ multiple %s none endevent " % NEVER
        + "event zwr1- multiple %s none endevent " % NEVER
        + "event zwr2+ multiple %s none endevent " % NEVER
        + "event zwr2- multiple %s none endevent " % NEVER
    )
    var root:MaszynaIncludeNode = MaszynaIncludeNode.new()
    root.autoload = false
    add_child(root)
    var tracks:Array[MaszynaTrackData] = []
    var track_rids:Array[RID] = []
    var power_sources:Array[MaszynaPowerSourceData] = []
    await MaszynaLegacyEventFactory.build(
        root, context.events, context.memcells, context.launchers, context.sounds, context.isolated_sections,
        tracks, track_rids, models, model_rids, power_sources
    )

    watch_signals(SceneryHUDMouseServer)
    SceneryHUDMouseServer.mouse_input(_motion(_camera.unproject_position(MODEL_POSITION)))
    # the tooltip: the model's name and the launcher's key, with Shift for the second event
    assert_signal_emitted_with_parameters(SceneryHUDMouseServer, "pickable_hovered", ["zwr1", "K / Shift+K"])
    SceneryHUDMouseServer.mouse_input(_click(true))
    SceneryHUDMouseServer.mouse_input(_motion(_camera.unproject_position(beside)))
    assert_false(SceneryHUDMouseServer.pickable_get_hovered().is_valid(), "zwr2 is out of its launcher's range")
    assert_signal_emitted(SceneryHUDMouseServer, "pickable_unhovered")

    assert_true(ScenarioEventServer.event_is_queued(ScenarioEventServer.event_get_rid_by_name(&"zwr1-")), "Shift fires event2")
    assert_false(ScenarioEventServer.event_is_queued(ScenarioEventServer.event_get_rid_by_name(&"zwr1+")))
    root.free()
    E3DRenderingServer.instance_free(lever)
    E3DRenderingServer.instance_free(far_lever)


func _model_data(model_name:String, position:Vector3) -> MaszynaModelData:
    var model_data:MaszynaModelData = MaszynaModelData.new()
    model_data.name = model_name
    model_data.position = position
    return model_data


func _parse(text:String) -> MaszynaImporterContext:
    var parser:MaszynaParser = MaszynaParser.new()
    parser.initialize(text.to_utf8_buffer())
    var context:MaszynaImporterContext = MaszynaImporterContext.new()
    parser.register_handler("event", func(p:MaszynaParser) -> Array: return EventImporter.new().import(p, context))
    parser.register_handler("node", func(p:MaszynaParser) -> Array: return NodeImporter.new().import(p, context))
    parser.parse()
    return context


func _press(operation:StringName) -> void:
    _presses.append(operation)


func _create_instance() -> RID:
    var instance:RID = E3DRenderingServer.instance_create(_create_model(), E3DRenderingServer.INSTANCER_OPTIMIZED, E3DRenderingServer.INSTANCE_KIND_STATIC)
    E3DRenderingServer.instance_set_scenario(instance, _camera.get_world_3d().scenario)
    E3DRenderingServer.instance_set_transform(instance, Transform3D(Basis(), MODEL_POSITION))
    E3DRenderingServer.instance_build(instance)
    return instance


func _create_model() -> E3DModel:
    var model:E3DModel = E3DModel.new()
    var mesh_submodel:E3DSubModel = E3DSubModel.new()
    mesh_submodel.resource_name = "lever"
    mesh_submodel.submodel_type = E3DSubModel.SUBMODEL_GL_TRIANGLES
    var mesh:ArrayMesh = ArrayMesh.new()
    mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, BoxMesh.new().get_mesh_arrays())
    mesh_submodel.mesh = mesh
    var submodels:Array[E3DSubModel] = [mesh_submodel]
    model.submodels = submodels
    return model


func _motion(position:Vector2) -> InputEventMouseMotion:
    var motion:InputEventMouseMotion = InputEventMouseMotion.new()
    motion.position = position
    return motion


func _click(shift:bool) -> InputEventMouseButton:
    var click:InputEventMouseButton = InputEventMouseButton.new()
    click.button_index = MOUSE_BUTTON_LEFT
    click.pressed = true
    click.shift_pressed = shift
    return click
