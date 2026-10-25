extends ReloadedGutTest
class_name MaszynaGutTest

## The core's test helpers and MaSzyna's: vehicles on the Mover, models of E3D, the game data's
## vehicles

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
## A vehicle is an object owned by RailVehicleServer and stepped by the server's own tick - it is
## not a node, so a test cannot put one in the tree. VehiclePhysicsNode is what brings a vehicle
## into being; autofree owns the node, so the vehicle goes away with the test.
## `description` is an authored vehicle (demo/tests/fixtures/*.tres); without one the vehicle comes
## up empty and the test adds the components it cares about. The vehicle has a front and a rear
## cabin; `driver` is the scenery's: the cabin a new person sits in as the DRIVER (DRIVER_HEAD the
## front one) - nobody's vehicle has no driver's cabin, so its cab cannot be activated
## (DynObj.cpp:1948-1964). The person is freed as the node leaves the tree; a test that needs it
## takes it from VehicleServer.vehicle_list_persons().
func build_vehicle(vehicle_id:String = "TestTrain", description:VehicleController = null,
        initial_velocity:float = 0.0,
        driver:MaszynaVehicleData.DriverType = MaszynaVehicleData.DriverType.DRIVER_NOBODY) -> VehicleController:
    return VehicleServer.vehicle_get_controller(
            build_vehicle_node(vehicle_id, description, initial_velocity, driver).get_vehicle_rid())


## The node itself, for a test that needs a NodePath to the vehicle (RailVehicle3D.controller_path).
func build_vehicle_node(vehicle_id:String = "TestTrain", description:VehicleController = null,
        initial_velocity:float = 0.0,
        driver:MaszynaVehicleData.DriverType = MaszynaVehicleData.DriverType.DRIVER_NOBODY) -> VehiclePhysicsNode:
    var physics_node: RailVehiclePhysicsNode = RailVehiclePhysicsNode.new()
    physics_node.controller = description
    physics_node.vehicle_id = vehicle_id
    physics_node.initial_velocity = initial_velocity
    # a rail vehicle, stepped by RailVehicleServer whether or not a RailVehicle3D places it
    add_child_autofree(physics_node)
    var vehicle_rid:RID = physics_node.get_vehicle_rid()
    RailVehicleServer.vehicle_add_front_cabin(vehicle_rid)
    RailVehicleServer.vehicle_add_rear_cabin(vehicle_rid)
    if driver == MaszynaVehicleData.DriverType.DRIVER_NOBODY:
        return physics_node
    var person:RID = PersonServer.person_create()
    # persons outlive vehicles: one left behind would sit in the next test's world
    physics_node.tree_exited.connect(PersonServer.person_free.bind(person))
    if driver == MaszynaVehicleData.DriverType.DRIVER_HEAD:
        RailVehicleServer.person_enter_front_cabin(person, vehicle_rid, VehiclePersonRole.VEHICLE_PERSON_ROLE_DRIVER)
    else:
        RailVehicleServer.person_enter_rear_cabin(person, vehicle_rid, VehiclePersonRole.VEHICLE_PERSON_ROLE_DRIVER)
    return physics_node


## What the original's AI keeps of the driver (MaszynaAIDriver.get_internal_state()); empty for a
## driver another implementation thinks for
func get_driver_internal_state(driver:RID) -> Dictionary:
    var ai:MaszynaAIDriver = DriverServer.driver_get_implementation(driver) as MaszynaAIDriver
    return ai.get_internal_state(driver) if ai else {}


## A power supply whose battery is `battery_voltage` [V] nominal - the low voltage a test's vehicle
## needs for its cab, which a bare vehicle has none of
func build_power_supply(battery_voltage:float) -> MaszynaMoverRailVehiclePowerSupply:
    var power_supply:MaszynaMoverRailVehiclePowerSupply = MaszynaMoverRailVehiclePowerSupply.new()
    power_supply.battery_voltage = battery_voltage
    return power_supply


## A RailVehicle3D standing on the track at the offset, with a mass - a vehicle with none integrates
## to NaN (test_rail_vehicle_at_rest.gd). It takes its controller within a few frames; the test frees
## it, before its physics node goes with autofree.
func build_rail_vehicle(vehicle_id:String, track_name:String, offset:float,
        driver:MaszynaVehicleData.DriverType = MaszynaVehicleData.DriverType.DRIVER_NOBODY) -> RailVehicle3D:
    var model:RailVehicleController = MaszynaMoverRailVehicleController.new()
    model.vehicle_id = vehicle_id
    model.mass = RAIL_VEHICLE_MASS
    model.type_name = "test"
    var physics_node:VehiclePhysicsNode = build_vehicle_node(vehicle_id, model, 0.0, driver)
    var vehicle:RailVehicle3D = RailVehicle3D.new()
    vehicle.start_track_name = track_name
    vehicle.start_track_offset = offset
    # a sibling of the physics node, which it takes the vehicle from as it enters the tree
    vehicle.controller_path = NodePath("../%s" % physics_node.name)
    add_child(vehicle)
    return vehicle


## A passenger car standing on the track at the offset, as build_rail_vehicle(), with doors and room
## for `capacity` passengers getting off and on `exchange_speed` a second through one open side
func build_passenger_car(vehicle_id:String, track_name:String, offset:float, capacity:float,
        exchange_speed:float, initial_velocity:float = 0.0) -> RailVehicle3D:
    var model:RailVehicleController = MaszynaMoverRailVehicleController.new()
    model.vehicle_id = vehicle_id
    model.mass = RAIL_VEHICLE_MASS
    model.type_name = "test"
    var doors:MaszynaMoverRailVehicleDoors = MaszynaMoverRailVehicleDoors.new()
    doors.max_shift = PASSENGER_CAR_DOOR_SHIFT
    model.add_component(doors)
    var load:MaszynaMoverRailVehicleLoad = MaszynaMoverRailVehicleLoad.new()
    load.max_load = capacity
    load.load_speed = exchange_speed
    load.unload_speed = exchange_speed
    var accepted:Array[String] = [PASSENGERS]
    load.accepted_loads = accepted
    model.add_component(load)
    var physics_node:VehiclePhysicsNode = build_vehicle_node(vehicle_id, model, initial_velocity)
    var vehicle:RailVehicle3D = RailVehicle3D.new()
    vehicle.start_track_name = track_name
    vehicle.start_track_offset = offset
    vehicle.controller_path = NodePath("../%s" % physics_node.name)
    add_child(vehicle)
    return vehicle


## An exterior model built in code, drawn as nodes - what a vehicle assembled by hand names its
## parts in (RailVehicle3D.model_instance_path): a transform submodel for every name, placed as
## given, under the submodel `parents` names for it or at the top. Added by the test to its vehicle.
func build_model_instance(submodels:Dictionary, parents:Dictionary) -> MaszynaE3DModelInstance:
    var built:Dictionary[String, MaszynaE3DSubModel] = {}
    for submodel_name:String in submodels:
        var submodel:MaszynaE3DSubModel = MaszynaE3DSubModel.new()
        submodel.resource_name = submodel_name
        submodel.submodel_type = MaszynaE3DSubModel.SUBMODEL_TRANSFORM
        submodel.transform = submodels[submodel_name]
        built[submodel_name] = submodel
    var top:Array[MaszynaE3DSubModel] = []
    for submodel_name:String in built:
        if parents.has(submodel_name):
            var parent:MaszynaE3DSubModel = built[parents[submodel_name]]
            var children:Array = parent.submodels
            children.append(built[submodel_name])
            parent.submodels = children
        else:
            top.append(built[submodel_name])
    var model:MaszynaE3DModel = MaszynaE3DModel.new()
    model.submodels = top
    var instance:MaszynaE3DModelInstance = MaszynaE3DModelInstance.new()
    instance.name = "Model"
    instance.instance_kind = ModelRenderingServer.INSTANCE_KIND_DYNAMIC
    instance.model = model
    return instance


## A MaSzyna vehicle built from the game data, added to the test and built - awaited. Drawn in detail
## (as nodes, which a test looks into) only once wait_detailed() says so: a fixture without model
## files is never drawn in detail
func spawn_maszyna_vehicle(data_path:String, file_name:String, skin:String, vehicle_id:String,
        driver_type:MaszynaVehicleData.DriverType = MaszynaVehicleData.DriverType.DRIVER_NOBODY) -> MaszynaRailVehicle3D:
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
    var rid: RID = MaszynaE3DRenderingServer.instance_create(build_lit_model(), ModelRenderingServer.INSTANCER_NODES, ModelRenderingServer.INSTANCE_KIND_STATIC)
    MaszynaE3DRenderingServer.instance_attach_object_instance_id(rid, parent.get_instance_id())
    MaszynaE3DRenderingServer.instance_build(rid)
    return rid


func build_lit_model() -> MaszynaE3DModel:
    var model: MaszynaE3DModel = MaszynaE3DModel.new()
    var light_on: MaszynaE3DSubModel = MaszynaE3DSubModel.new()
    light_on.resource_name = "light_on00"
    light_on.submodel_type = MaszynaE3DSubModel.SUBMODEL_TRANSFORM
    var mesh_submodel: MaszynaE3DSubModel = MaszynaE3DSubModel.new()
    mesh_submodel.resource_name = "mesh"
    mesh_submodel.submodel_type = MaszynaE3DSubModel.SUBMODEL_GL_TRIANGLES
    var mesh: ArrayMesh = ArrayMesh.new()
    mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, BoxMesh.new().get_mesh_arrays())
    mesh_submodel.mesh = mesh
    light_on.submodels = [mesh_submodel]
    model.submodels = [light_on]
    var light_definition: MaszynaE3DModelLightDefinition = MaszynaE3DModelLightDefinition.new()
    light_definition.on_submodel_path = NodePath("light_on00")
    model.register_light("00", light_definition)
    return model
