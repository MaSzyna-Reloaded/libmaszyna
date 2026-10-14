extends MaszynaGutTest

## RailVehicleRenderingServer draws a rail vehicle wherever RailVehicleServer places it - the nodes
## mounted on it ride along - poses and hides the submodels of its models, and lets go of it with
## the vehicle.

const TRACK_NAME:String = "rendering_track"
const TRACK_LENGTH:float = 100.0
const OFFSET:float = 30.0
const OTHER_OFFSET:float = 10.0
const TOLERANCE:float = 0.001
const SETTLE_FRAMES:int = 3
const VEHICLE_FIXTURE:String = "res://tests/fixtures/test_vehicle.fiz"
const COUPLING_WITH_BRAKE_HOSE:int = (RailVehicleController.COUPLING_FLAG_COUPLER
        | RailVehicleController.COUPLING_FLAG_BRAKEHOSE)
## The low-poly interior's cabs (LowPolyIntCabs, DynObj.cpp:2383-2391)
const LOW_POLY_CABS:Array[String] = ["cab0", "cab1", "cab2"]

enum HoseGeometry { HANGING_ONLY, CONNECTED }

## How far a test moves its vehicle along the track [m]
const MOVE_DISTANCE:float = 5.0
## The fabricated vehicle with models (demo/tests/fixtures/dynamic/test/synthetic_v1)
const FIXTURES_GAME_DIR:String = "res://tests/fixtures"
## Where a camera stands beyond the streaming's draw distance and its margin (a quarter of it), as
## a share of the draw distance
const BEYOND_DRAW_DISTANCE:float = 2.0
## How long the vehicles' sweep may take to build or free a vehicle's models [s]
const MODELS_TIMEOUT:float = 5.0

var _track:RID
var _vehicle:RailVehicle3D
var _other_vehicle:RailVehicle3D
var _model:E3DModelInstance
var _other_model:E3DModelInstance


func before_each() -> void:
    _track = build_track(TRACK_NAME, TRACK_LENGTH)


func after_each() -> void:
    if is_instance_valid(_vehicle):
        free_rail_vehicle(_vehicle)
    if is_instance_valid(_other_vehicle):
        free_rail_vehicle(_other_vehicle)
    TrackServer.track_free(_track)
    TrackServer.topology_rebuild()


func _coupler_model(hose:String, geometry:HoseGeometry) -> E3DModelInstance:
    var submodels:Dictionary = {
        "coupler1_off": Transform3D(),
        "coupler1_on": Transform3D(),
        "coupler2_off": Transform3D(),
        "coupler2_on": Transform3D(),
        hose + "_off": Transform3D(),
    }
    if geometry == HoseGeometry.CONNECTED:
        submodels[hose + "_on"] = Transform3D()
    var no_parents:Dictionary = {}
    var model:E3DModelInstance = build_model_instance(submodels, no_parents)
    for submodel:E3DSubModel in model.model.submodels:
        if submodel.resource_name.ends_with("_on"):
            submodel.dynamic_hidden = true
    return model


func _build_coupler_vehicle(
        vehicle_name:String, offset:float, description:VehicleController, model:E3DModelInstance) -> RailVehicle3D:
    var physics_node:VehiclePhysicsNode = build_vehicle_node(vehicle_name, description)
    var vehicle:RailVehicle3D = RailVehicle3D.new()
    vehicle.start_track_name = TRACK_NAME
    vehicle.start_track_offset = offset
    vehicle.controller_path = NodePath("../%s" % physics_node.name)
    vehicle.add_child(model)
    vehicle.model_instance_path = NodePath("Model")
    add_child(vehicle)
    return vehicle


func test_the_node_stands_where_the_vehicle_is_placed() -> void:
    _vehicle = build_rail_vehicle("RenderingPlaced", TRACK_NAME, OFFSET)
    await wait_idle_frames(SETTLE_FRAMES)

    var rid:RID = _vehicle.get_rid()
    assert_true(RailVehicleRenderingServer.vehicle_get_transform(rid).is_equal_approx(
            RailVehicleServer.vehicle_get_transform(rid)), "the vehicle is drawn where it is placed")
    assert_true(_vehicle.global_transform.is_equal_approx(RailVehicleServer.vehicle_get_transform(rid)),
            "the node of a vehicle assembled by hand rides on it")

    RailVehicleServer.vehicle_move(rid, MOVE_DISTANCE)
    assert_true(_vehicle.global_transform.is_equal_approx(RailVehicleServer.vehicle_get_transform(rid)),
            "and follows it at once when it moves")


## A node of another layer - a cab, a sound emitter - rides on the vehicle while it is mounted
func test_a_mounted_node_rides_on_the_vehicle() -> void:
    _vehicle = build_rail_vehicle("RenderingMount", TRACK_NAME, OFFSET)
    await wait_idle_frames(SETTLE_FRAMES)
    var rid:RID = _vehicle.get_rid()
    var mount:Node3D = add_child_autofree(Node3D.new())

    RailVehicleRenderingServer.vehicle_mount_node(rid, mount.get_instance_id())
    assert_true(mount.global_transform.is_equal_approx(RailVehicleServer.vehicle_get_transform(rid)),
            "it is put where the vehicle stands")

    RailVehicleServer.vehicle_move(rid, MOVE_DISTANCE)
    assert_true(mount.global_transform.is_equal_approx(RailVehicleServer.vehicle_get_transform(rid)),
            "and follows the vehicle when it moves")

    RailVehicleRenderingServer.vehicle_unmount_node(rid, mount.get_instance_id())
    var left_at:Transform3D = mount.global_transform
    RailVehicleServer.vehicle_move(rid, MOVE_DISTANCE)
    assert_true(mount.global_transform.is_equal_approx(left_at), "unmounted, it stays where it was")


## The player finds a vehicle by the area around its model, which names the vehicle - there is no
## node of it to find
func test_the_detection_area_names_its_vehicle() -> void:
    var previous_game_dir:String = UserSettings.get_maszyna_game_dir()
    UserSettings.save_maszyna_game_dir(FIXTURES_GAME_DIR)
    var vehicle:MaszynaRailVehicle3D = await spawn_maszyna_vehicle(
            "dynamic/test/synthetic_v1", "synthetic", "", "rendering_detection")
    if not await wait_detailed(vehicle):
        return
    await wait_physics_frames(SETTLE_FRAMES)

    # the area is a box the size of the model, where the model is drawn
    var rid:RID = vehicle.get_rid()
    var query:PhysicsPointQueryParameters3D = PhysicsPointQueryParameters3D.new()
    query.position = (RailVehicleRenderingServer.vehicle_get_transform(rid)
            * RailVehicleRenderingServer.vehicle_get_appearance(rid).model_transform
            * E3DRenderingServer.instance_get_aabb(RailVehicleRenderingServer.vehicle_get_model(rid)).get_center())
    query.collide_with_areas = true
    query.collide_with_bodies = false
    var hits:Array[Dictionary] = get_viewport().world_3d.direct_space_state.intersect_point(query)

    assert_eq(hits.size(), 1, "the vehicle's detection area is found where the vehicle stands")
    if hits:
        assert_eq(RailVehicleRenderingServer.detection_area_get_vehicle(hits[0]["rid"]), rid)
    # the vehicle goes before the game directory it was read from: the game's data read again
    # would build it anew
    vehicle.free()
    UserSettings.save_maszyna_game_dir(previous_game_dir)


## "Edit FIZ": an editable vehicle's models stand as nodes under a holder the Scene dock shows - not
## an internal child of the vehicle's node - and are hidden again with it off
func test_an_editable_vehicle_shows_its_model_as_nodes() -> void:
    var previous_game_dir:String = UserSettings.get_maszyna_game_dir()
    UserSettings.save_maszyna_game_dir(FIXTURES_GAME_DIR)
    var vehicle:MaszynaRailVehicle3D = await spawn_maszyna_vehicle(
            "dynamic/test/synthetic_v1", "synthetic", "", "rendering_editable")
    if not await wait_detailed(vehicle):
        return
    var rid:RID = vehicle.get_rid()
    assert_eq(vehicle.get_child_count(false), 0, "the nodes of the model are internal")

    vehicle.set_editable_in_editor(true)

    assert_true(RailVehicleRenderingServer.vehicle_is_editable(rid))
    assert_true(RailVehicleRenderingServer.vehicle_is_detailed(rid), "drawn in detail wherever the camera is")
    assert_eq(vehicle.get_child_count(false), 1, "the holder of the model is shown")
    if vehicle.get_child_count(false):
        assert_gt(vehicle.get_child(0, false).get_child_count(), 0, "the model stands as nodes under it")

    vehicle.set_editable_in_editor(false)

    assert_false(RailVehicleRenderingServer.vehicle_is_editable(rid))
    assert_eq(vehicle.get_child_count(false), 0, "the holder is hidden again")
    # the vehicle goes before the game directory it was read from
    vehicle.free()
    UserSettings.save_maszyna_game_dir(previous_game_dir)


func _has_model(vehicle:RID) -> bool:
    return RailVehicleRenderingServer.vehicle_get_model(vehicle).is_valid()


func _has_no_model(vehicle:RID) -> bool:
    return not RailVehicleRenderingServer.vehicle_get_model(vehicle).is_valid()


## A vehicle beyond the streaming's draw distance is not drawn at all: its models are built when the
## camera comes within it and freed when the camera goes beyond it again - a scenery's hundreds of
## vehicles all built at load spent it on vehicles nobody saw. With lazy loading its model file is
## loaded only for the build
func test_a_vehicle_is_built_only_within_the_draw_distance() -> void:
    await _drive_camera_past_a_vehicle(true, "rendering_draw_distance")


## Without lazy loading the model file is loaded with the vehicle and kept; only the models built
## from it follow the draw distance
func test_a_vehicle_loads_its_model_at_once_without_lazy_loading() -> void:
    await _drive_camera_past_a_vehicle(false, "rendering_eager")


func _drive_camera_past_a_vehicle(lazy_loading:bool, vehicle_id:String) -> void:
    var previous_lazy_loading:bool = ResourceLazyLoader.lazy_loading
    ResourceLazyLoader.lazy_loading = lazy_loading
    var previous_game_dir:String = UserSettings.get_maszyna_game_dir()
    UserSettings.save_maszyna_game_dir(FIXTURES_GAME_DIR)
    var camera:Camera3D = add_child_autoqfree(Camera3D.new())
    var far:Vector3 = Vector3(BEYOND_DRAW_DISTANCE * SceneryStreamingServer.streaming_get_draw_distance(), 0.0, 0.0)
    camera.global_position = far
    SceneryStreamingServer.streaming_set_camera(camera.get_instance_id())
    var vehicle:MaszynaRailVehicle3D = MaszynaRailVehicle3D.new()
    vehicle.data_path = "dynamic/test/synthetic_v1"
    vehicle.file_name = "synthetic"
    vehicle.vehicle_id = vehicle_id
    add_child(vehicle)
    await vehicle.vehicle_built
    await wait_idle_frames(SETTLE_FRAMES)
    var rid:RID = vehicle.get_rid()
    var appearance:RailVehicleAppearance = RailVehicleRenderingServer.vehicle_get_appearance(rid)
    # the same key is the same resource; this registration only lets the test look at it
    var model_resource:RID = ResourceLazyLoader.resource_register(
            appearance.data_path.path_join(appearance.model_filename), Callable())

    assert_true(VehicleServer.vehicle_is_simulation_ready(rid), "simulated wherever the camera is")
    assert_false(_has_model(rid), "beyond the draw distance it has no model")
    assert_eq(ResourceLazyLoader.resource_is_resident(model_resource), not lazy_loading,
            "its model file loaded beyond the draw distance")

    camera.global_position = vehicle.global_position
    await wait_until(_has_model.bind(rid), MODELS_TIMEOUT)
    assert_true(_has_model(rid), "within the draw distance its model is built")
    assert_true(ResourceLazyLoader.resource_is_resident(model_resource), "built without holding its model file")

    camera.global_position = far
    await wait_until(_has_no_model.bind(rid), MODELS_TIMEOUT)
    assert_false(_has_model(rid), "beyond it again its model is freed")
    assert_eq(ResourceLazyLoader.resource_is_resident(model_resource), not lazy_loading,
            "its model file kept beyond the draw distance")

    ResourceLazyLoader.resource_free(model_resource)
    SceneryStreamingServer.streaming_set_camera(0)
    # the vehicle goes before the game directory it was read from
    vehicle.free()
    UserSettings.save_maszyna_game_dir(previous_game_dir)
    ResourceLazyLoader.lazy_loading = previous_lazy_loading


func test_a_rebuilt_model_is_announced() -> void:
    _vehicle = build_rail_vehicle("RenderingRebuilt", TRACK_NAME, OFFSET)
    var submodels:Dictionary = {"body": Transform3D()}
    var no_parents:Dictionary = {}
    var model:E3DModelInstance = build_model_instance(submodels, no_parents)
    _vehicle.add_child(model)
    _vehicle.model_instance_path = NodePath("Model")
    await wait_idle_frames(SETTLE_FRAMES)
    watch_signals(RailVehicleRenderingServer)

    model.reload()

    assert_signal_emitted_with_parameters(RailVehicleRenderingServer, "vehicle_model_built", [_vehicle.get_rid()])


func test_the_low_poly_interior_hides_the_cab_whose_interior_is_drawn() -> void:
    _vehicle = build_rail_vehicle("RenderingCabs", TRACK_NAME, OFFSET, MaszynaDynamicData.DriverType.DRIVER_HEAD)
    var exterior_submodels:Dictionary = {"body": Transform3D()}
    var cab_submodels:Dictionary = {}
    for cab:String in LOW_POLY_CABS:
        cab_submodels[cab] = Transform3D()
    var no_parents:Dictionary = {}
    var exterior:E3DModelInstance = build_model_instance(exterior_submodels, no_parents)
    var low_poly:E3DModelInstance = build_model_instance(cab_submodels, no_parents)
    low_poly.name = "LowPoly"
    _vehicle.add_child(exterior)
    _vehicle.add_child(low_poly)
    _vehicle.model_instance_path = NodePath("Model")
    _vehicle.low_poly_cabin_path = NodePath("LowPoly")
    await wait_idle_frames(SETTLE_FRAMES)
    RailVehicleServer.vehicle_add_machine_room(_vehicle.get_rid())

    RailVehicleRenderingServer.vehicle_set_visible_low_poly_cabins(_vehicle.get_rid(), false)

    assert_true((low_poly.get_node("cab0") as Node3D).visible, "the machine room stays")
    assert_false((low_poly.get_node("cab1") as Node3D).visible, "the driver's cab, whose interior is drawn in its place, is hidden")
    assert_true((low_poly.get_node("cab2") as Node3D).visible, "the other cab stays")

    # the hidden cab follows the driver's (Train.cpp:8516 CabChange: 1 -> 0, the machine room)
    RailVehicleServer.person_change_cabin(get_vehicle_driver(_vehicle.get_rid()),
            RailVehicleServer.CABIN_CHANGE_BACKWARD)
    assert_false((low_poly.get_node("cab0") as Node3D).visible, "the cab moved to is hidden")
    assert_true((low_poly.get_node("cab1") as Node3D).visible, "and the one left is shown again")

    RailVehicleRenderingServer.vehicle_set_visible_low_poly_cabins(_vehicle.get_rid(), true)
    assert_true((low_poly.get_node("cab0") as Node3D).visible, "with no interior drawn every low-poly cab is shown")


func test_coupling_shows_the_owned_submodel_and_keeps_missing_hose_geometry_off() -> void:
    var description:VehicleController = FizVehicleBuilder.build_description_at(VEHICLE_FIXTURE)
    _model = _coupler_model("cpneumatic2", HoseGeometry.HANGING_ONLY)
    _other_model = _coupler_model("cpneumatic1", HoseGeometry.CONNECTED)
    _vehicle = _build_coupler_vehicle("RenderingCouplerFirst", OFFSET, description, _model)
    _other_vehicle = _build_coupler_vehicle("RenderingCouplerSecond", OTHER_OFFSET, description, _other_model)
    await wait_idle_frames(SETTLE_FRAMES)

    var first_on:Node3D = _model.get_node("coupler2_on")
    var first_off:Node3D = _model.get_node("coupler2_off")
    var second_on:Node3D = _other_model.get_node("coupler1_on")
    var second_off:Node3D = _other_model.get_node("coupler1_off")
    assert_false(first_on.visible)
    assert_true(first_off.visible)
    assert_false(second_on.visible)
    assert_true(second_off.visible)

    RailVehicleServer.vehicle_couple(
            _vehicle.get_rid(), RailVehicleController.COUPLER_END_REAR,
            _other_vehicle.get_rid(), RailVehicleController.COUPLER_END_FRONT,
            COUPLING_WITH_BRAKE_HOSE)

    assert_eq(int(first_on.visible) + int(second_on.visible), 1,
            "exactly the coupling owner should draw the connected mechanical coupler")
    assert_eq(int(first_off.visible) + int(second_off.visible), 1,
            "the other vehicle should retain its hanging coupler when it has no xon variant")
    assert_true((_model.get_node("cpneumatic2_off") as Node3D).visible,
            "a vehicle with no connected hose geometry should retain its hanging hose")
    var second_hose_on:Node3D = _other_model.get_node("cpneumatic1_on")
    var second_hose_off:Node3D = _other_model.get_node("cpneumatic1_off")
    assert_true(second_hose_on.visible, "the other vehicle should draw its connected hose")
    assert_false(second_hose_off.visible)

    var controller:VehicleController = VehicleServer.vehicle_get_controller(_vehicle.get_rid())
    controller.uncouple(RailVehicleController.COUPLER_END_REAR)
    assert_false(second_hose_on.visible, "uncoupling should hide the other vehicle's connected hose")
    assert_true(second_hose_off.visible)

    RailVehicleServer.vehicle_couple(
            _vehicle.get_rid(), RailVehicleController.COUPLER_END_REAR,
            _other_vehicle.get_rid(), RailVehicleController.COUPLER_END_FRONT,
            COUPLING_WITH_BRAKE_HOSE)
    assert_true(second_hose_on.visible, "recoupling should restore the other vehicle's connected hose")
    assert_false(second_hose_off.visible)

    free_rail_vehicle(_vehicle)
    _vehicle = null
    assert_false(second_on.visible, "the remaining vehicle should stop drawing the connected coupler")
    assert_true(second_off.visible, "the remaining vehicle should restore its hanging coupler")


func test_the_vehicle_is_let_go_of_when_freed() -> void:
    _vehicle = build_rail_vehicle("RenderingFreed", TRACK_NAME, OFFSET)
    await wait_idle_frames(SETTLE_FRAMES)
    var rid:RID = _vehicle.get_rid()
    assert_true(RailVehicleRenderingServer.vehicle_is_attached(rid))

    free_rail_vehicle(_vehicle)

    assert_false(RailVehicleRenderingServer.vehicle_is_attached(rid), "freed with the vehicle")
