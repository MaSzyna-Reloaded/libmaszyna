extends GutTest
class_name MaszynaGutTest

## Standard gauge of build_track() [m]
const BUILT_TRACK_GAUGE:float = 1.435
## build_rail_vehicle()'s mass [kg]
const RAIL_VEHICLE_MASS:float = 74000.0
## build_passenger_car()'s door travel [m] - a car without it has no doors (Mover.cpp:7920)
const PASSENGER_CAR_DOOR_SHIFT:float = 0.5
## The load passenger cars carry, as the data names it (FIZ LoadAccepted, the original's station.cpp)
const PASSENGERS:String = "passengers"

## Simulated seconds a spawned vehicle may take to be drawn in detail - its steps place it, its frames
## look at its detail a few times a second (RailVehicleRenderingServer)
const DETAIL_TIMEOUT:float = 5.0
## Real seconds a vehicle of the game data may take to be built - parsed and configured, a few
## frames of work
const BUILD_TIMEOUT:float = 5.0
## Real seconds a scenery fixture may take to load: parsing and building is the machine's work, not
## simulated time - a fixture loads in under 0.1 s, this only catches a hang
const LOAD_TIMEOUT:float = 5.0
## A few of the streaming's camera watch intervals (SceneryStreamingServer WATCH_INTERVAL_SEC 0.1 s)
const STREAMING_WATCH_SEC:float = 0.3
## One step of the test's clock [simulated s]: the simulation's tick to come (#301, 30 Hz). The run
## has no SimulationRuntime - no frame moves the simulation; a test steps it tick by tick, as a step
## debugger does, and what happens does not depend on how fast the machine is
const TICK:float = 1.0 / 30.0


## A script leaves the simulation as the hook set it: running, at speed 1, there already - a speed
## only set back (simulation_speed) leaves the clock at the old one, and every script after it runs
## at that until the speed change closes (FINDINGS.md); SimulationServer.simulation_reset_speed()
## sets both. Asserted here, so the script that leaves it is the one that goes red.
func after_all() -> void:
    assert_false(SimulationServer.simulation_is_paused(), "the script left the simulation paused")
    assert_eq(SimulationServer.simulation_speed, 1.0, "the script left the simulation's speed set")
    assert_eq(SimulationServer.simulation_get_current_speed(), 1.0,
            "the script left the simulation's clock at another speed")


## The ticks `seconds` of simulated time take, the last one whole
static func ticks(seconds:float) -> int:
    return ceili(seconds / TICK)


## `count` ticks of the simulation, one after the other - a frame between them for what runs on the
## frames (the keys, a knob a key holds)
func step(count:int) -> void:
    for _tick:int in count:
        SimulationServer.simulation_advance(TICK)
        await Engine.get_main_loop().process_frame


## Steps the simulation tick by tick until `done`, for at most `simulated_seconds` - worked out from
## the data of what is waited for. Not coming about by then fails the test there, saying `what`;
## true when it came
func wait_simulated_until(done:Callable, simulated_seconds:float, what:String) -> bool:
    for _tick:int in ticks(simulated_seconds):
        if done.call():
            return true
        SimulationServer.simulation_advance(TICK)
        await Engine.get_main_loop().process_frame
    if done.call():
        return true
    fail_test("%s did not come about within %.2f simulated seconds (%d ticks)" % [
            what, simulated_seconds, ticks(simulated_seconds)])
    return false


## The loading that `loaded` announces is done; one that does not end within LOAD_TIMEOUT fails the
## test there, saying `what`
func wait_loaded(loaded:Signal, what:String) -> bool:
    if await wait_for_signal(loaded, LOAD_TIMEOUT):
        return true
    fail_test("%s did not load within %.0f s" % [what, LOAD_TIMEOUT])
    return false


func wait_idle_frames(frames, message = ""):
    while frames > 0:
        await Engine.get_main_loop().process_frame
        frames -= 1


## The streaming looks at its camera, anchor and content every 0.1 s of real time
## (SceneryStreamingServer WATCH_INTERVAL_SEC) and applies what it planned per frame: a change of
## them is waited for by both
func wait_streaming(frames:int) -> void:
    await wait_seconds(STREAMING_WATCH_SEC)
    await wait_idle_frames(frames)


## A vehicle is an object owned by RailVehicleServer and stepped by the server's own tick - it is
## not a node, so a test cannot put one in the tree. VehiclePhysicsNode is what brings a vehicle
## into being; autofree owns the node, so the vehicle goes away with the test.
## `description` is an authored vehicle (demo/tests/fixtures/*.tres); without one the vehicle comes
## up empty and the test adds the components it cares about. The vehicle has a front and a rear
## cabin; `driver` is the scenery's: the cabin a new person sits in as the DRIVER (DRIVER_HEAD the
## front one) - nobody's vehicle has no driver's cabin, so its cab cannot be activated
## (DynObj.cpp:1948-1964). The person is freed as the node leaves the tree; a test that needs it
## takes it from VehicleServer.vehicle_list_persons().
func build_vehicle(train_id:String = "TestTrain", description:VehicleController = null,
        initial_velocity:float = 0.0,
        driver:MaszynaDynamicData.DriverType = MaszynaDynamicData.DriverType.DRIVER_NOBODY) -> VehicleController:
    return VehicleServer.vehicle_get_controller(
            build_vehicle_node(train_id, description, initial_velocity, driver).get_vehicle_rid())


## The node itself, for a test that needs a NodePath to the vehicle (RailVehicle3D.controller_path).
func build_vehicle_node(train_id:String = "TestTrain", description:VehicleController = null,
        initial_velocity:float = 0.0,
        driver:MaszynaDynamicData.DriverType = MaszynaDynamicData.DriverType.DRIVER_NOBODY) -> VehiclePhysicsNode:
    var physics_node: RailVehiclePhysicsNode = RailVehiclePhysicsNode.new()
    physics_node.controller = description
    physics_node.vehicle_id = train_id
    physics_node.initial_velocity = initial_velocity
    # a rail vehicle, stepped by RailVehicleServer whether or not a RailVehicle3D places it
    add_child_autofree(physics_node)
    var vehicle_rid:RID = physics_node.get_vehicle_rid()
    RailVehicleServer.vehicle_add_front_cabin(vehicle_rid)
    RailVehicleServer.vehicle_add_rear_cabin(vehicle_rid)
    if driver == MaszynaDynamicData.DriverType.DRIVER_NOBODY:
        return physics_node
    var person:RID = PersonServer.person_create()
    # persons outlive vehicles: one left behind would sit in the next test's world
    physics_node.tree_exited.connect(PersonServer.person_free.bind(person))
    if driver == MaszynaDynamicData.DriverType.DRIVER_HEAD:
        RailVehicleServer.person_enter_front_cabin(person, vehicle_rid, VehiclePersonRole.VEHICLE_PERSON_ROLE_DRIVER)
    else:
        RailVehicleServer.person_enter_rear_cabin(person, vehicle_rid, VehiclePersonRole.VEHICLE_PERSON_ROLE_DRIVER)
    return physics_node


## The person becomes a driver thinking with `implementation` (null: no driver any more): it is
## registered with DriverServer under a name of its own, which the driver is declared with
func attach_driver_implementation(driver:RID, implementation:DriverImplementation) -> void:
    if not implementation:
        DriverServer.driver_attach_implementation(driver, &"")
        return
    var implementation_name:StringName = StringName("test_%d" % implementation.get_instance_id())
    DriverServer.implementation_register(implementation_name, implementation)
    DriverServer.driver_attach_implementation(driver, implementation_name)


## The person build_vehicle() seated as the DRIVER of the vehicle
func get_vehicle_driver(vehicle_rid:RID) -> RID:
    var drivers:Array[VehiclePerson] = VehicleServer.vehicle_list_persons(
            vehicle_rid, VehiclePersonRole.VEHICLE_PERSON_ROLE_DRIVER)
    return drivers[0].get_person()


## A power supply whose battery is `battery_voltage` [V] nominal - the low voltage a test's vehicle
## needs for its cab, which a bare vehicle has none of
func build_power_supply(battery_voltage:float) -> MoverRailVehiclePowerSupply:
    var power_supply:MoverRailVehiclePowerSupply = MoverRailVehiclePowerSupply.new()
    power_supply.battery_voltage = battery_voltage
    return power_supply


## A straight track along +X to stand vehicles on; the test frees it (TrackServer.track_free(), then
## topology_rebuild())
func build_track(track_name:String, length:float) -> RID:
    var curve:TrackCurve = TrackCurve.new()
    curve.p2 = Vector3(length, 0.0, 0.0)
    var track:RID = TrackServer.track_create()
    TrackServer.track_update_curves(track, curve, null)
    TrackServer.track_update(track, TrackServer.TRACK_NORMAL, track_name, BUILT_TRACK_GAUGE)
    TrackServer.topology_rebuild()
    return track


## A RailVehicle3D standing on the track at the offset, with a mass - a vehicle with none integrates
## to NaN (test_rail_vehicle_at_rest.gd). It takes its controller within a few frames; the test frees
## it, before its physics node goes with autofree.
func build_rail_vehicle(train_id:String, track_name:String, offset:float,
        driver:MaszynaDynamicData.DriverType = MaszynaDynamicData.DriverType.DRIVER_NOBODY) -> RailVehicle3D:
    var model:RailVehicleController = MoverRailVehicleController.new()
    model.vehicle_id = train_id
    model.mass = RAIL_VEHICLE_MASS
    model.type_name = "test"
    var physics_node:VehiclePhysicsNode = build_vehicle_node(train_id, model, 0.0, driver)
    var vehicle:RailVehicle3D = RailVehicle3D.new()
    vehicle.start_track_name = track_name
    vehicle.start_track_offset = offset
    # a sibling of the physics node, which it takes the vehicle from as it enters the tree
    vehicle.controller_path = NodePath("../%s" % physics_node.name)
    add_child(vehicle)
    return vehicle


## A passenger car standing on the track at the offset, as build_rail_vehicle(), with doors and room
## for `capacity` passengers getting off and on `exchange_speed` a second through one open side
func build_passenger_car(train_id:String, track_name:String, offset:float, capacity:float,
        exchange_speed:float, initial_velocity:float = 0.0) -> RailVehicle3D:
    var model:RailVehicleController = MoverRailVehicleController.new()
    model.vehicle_id = train_id
    model.mass = RAIL_VEHICLE_MASS
    model.type_name = "test"
    var doors:MoverRailVehicleDoors = MoverRailVehicleDoors.new()
    doors.max_shift = PASSENGER_CAR_DOOR_SHIFT
    model.add_component(doors)
    var load:MoverRailVehicleLoad = MoverRailVehicleLoad.new()
    load.max_load = capacity
    load.load_speed = exchange_speed
    load.unload_speed = exchange_speed
    var accepted:Array[String] = [PASSENGERS]
    load.accepted_loads = accepted
    model.add_component(load)
    var physics_node:VehiclePhysicsNode = build_vehicle_node(train_id, model, initial_velocity)
    var vehicle:RailVehicle3D = RailVehicle3D.new()
    vehicle.start_track_name = track_name
    vehicle.start_track_offset = offset
    vehicle.controller_path = NodePath("../%s" % physics_node.name)
    add_child(vehicle)
    return vehicle


## The vehicle build_rail_vehicle() made, gone: the node first - it lets go of its controller - then
## its physics node, which takes the vehicle out of RailVehicleServer (vehicle_freed)
func free_rail_vehicle(vehicle:RailVehicle3D) -> void:
    var physics_node:Node = vehicle.get_node(vehicle.controller_path)
    remove_child(vehicle)
    vehicle.free()
    physics_node.free()


## An exterior model built in code, drawn as nodes - what a vehicle assembled by hand names its
## parts in (RailVehicle3D.model_instance_path): a transform submodel for every name, placed as
## given, under the submodel `parents` names for it or at the top. Added by the test to its vehicle.
func build_model_instance(submodels:Dictionary, parents:Dictionary) -> E3DModelInstance:
    var built:Dictionary[String, E3DSubModel] = {}
    for submodel_name:String in submodels:
        var submodel:E3DSubModel = E3DSubModel.new()
        submodel.resource_name = submodel_name
        submodel.submodel_type = E3DSubModel.SUBMODEL_TRANSFORM
        submodel.transform = submodels[submodel_name]
        built[submodel_name] = submodel
    var top:Array[E3DSubModel] = []
    for submodel_name:String in built:
        if parents.has(submodel_name):
            var parent:E3DSubModel = built[parents[submodel_name]]
            var children:Array = parent.submodels
            children.append(built[submodel_name])
            parent.submodels = children
        else:
            top.append(built[submodel_name])
    var model:E3DModel = E3DModel.new()
    model.submodels = top
    var instance:E3DModelInstance = E3DModelInstance.new()
    instance.name = "Model"
    instance.instance_kind = E3DRenderingServer.INSTANCE_KIND_DYNAMIC
    instance.model = model
    return instance


## A MaSzyna vehicle built from the game data, added to the test and built - awaited. Drawn in detail
## (as nodes, which a test looks into) only once wait_detailed() says so: a fixture without model
## files is never drawn in detail
func spawn_maszyna_vehicle(data_path:String, file_name:String, skin:String, vehicle_id:String,
        driver_type:MaszynaDynamicData.DriverType = MaszynaDynamicData.DriverType.DRIVER_NOBODY) -> MaszynaRailVehicle3D:
    var vehicle:MaszynaRailVehicle3D = MaszynaRailVehicle3D.new()
    vehicle.driver_type = driver_type
    vehicle.data_path = data_path
    vehicle.file_name = file_name
    vehicle.skin = skin
    vehicle.vehicle_id = vehicle_id
    add_child(vehicle)
    if not await wait_for_signal(vehicle.vehicle_built, BUILD_TIMEOUT):
        fail_test("%s/%s was not built within %.0f s" % [data_path, file_name, BUILD_TIMEOUT])
        return vehicle
    # resumed inside the vehicle's own emission, the test would run on its stack - where the
    # vehicle is locked and cannot be freed; the test goes on from the next frame
    await wait_idle_frames(1)
    return vehicle


## The vehicle drawn in detail, as nodes - only near the streaming camera: one stands at it for as
## long as that takes, unless the test has its own. Placed by the simulation's step, drawn by the
## frames - a step is both; not coming about fails the test there
func wait_detailed(vehicle:MaszynaRailVehicle3D) -> bool:
    var has_camera:bool = SceneryStreamingServer.streaming_has_camera()
    if not has_camera:
        var camera:Camera3D = add_child_autoqfree(Camera3D.new())
        SceneryStreamingServer.streaming_set_camera(camera.get_instance_id())
    var detailed:bool = await wait_simulated_until(
            RailVehicleRenderingServer.vehicle_is_detailed.bind(vehicle.get_rid()), DETAIL_TIMEOUT,
            "%s drawn in detail" % vehicle.vehicle_id)
    if not has_camera:
        SceneryStreamingServer.streaming_set_camera(0)
    return detailed


## A static E3D instance of build_lit_model(), drawn as nodes under `parent` and built - its
## `light_on00` submodel is shown while light 00 is on
func build_lit_instance(parent: Node3D) -> RID:
    var rid: RID = E3DRenderingServer.instance_create(build_lit_model(), E3DRenderingServer.INSTANCER_NODES, E3DRenderingServer.INSTANCE_KIND_STATIC)
    E3DRenderingServer.instance_attach_object_instance_id(rid, parent.get_instance_id())
    E3DRenderingServer.instance_build(rid)
    return rid


func build_lit_model() -> E3DModel:
    var model: E3DModel = E3DModel.new()
    var light_on: E3DSubModel = E3DSubModel.new()
    light_on.resource_name = "light_on00"
    light_on.submodel_type = E3DSubModel.SUBMODEL_TRANSFORM
    var mesh_submodel: E3DSubModel = E3DSubModel.new()
    mesh_submodel.resource_name = "mesh"
    mesh_submodel.submodel_type = E3DSubModel.SUBMODEL_GL_TRIANGLES
    var mesh: ArrayMesh = ArrayMesh.new()
    mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, BoxMesh.new().get_mesh_arrays())
    mesh_submodel.mesh = mesh
    light_on.submodels = [mesh_submodel]
    model.submodels = [light_on]
    var light_definition: E3DModelLightDefinition = E3DModelLightDefinition.new()
    light_definition.on_submodel_path = NodePath("light_on00")
    model.register_light("00", light_definition)
    return model
