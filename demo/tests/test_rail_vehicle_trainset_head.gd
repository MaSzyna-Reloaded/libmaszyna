extends MaszynaGutTest

## A trainset's head (RailVehicleServer.trainset_get_head()), the order of its vehicles from it
## (trainset_get_vehicles()) and which of them stand the other way round in it
## (vehicle_is_reversed_in_trainset()) - recomputed by the server with every coupling, for both
## trainsets of it. The head is always an end vehicle:
## 1. the vehicle with a driving cab at an end whose cab a driver occupies or is in command - the
##    latest one when both ends are
## 2. else the one vehicle with a driving cab at an end, pulling or pushing
## 3. else the end nearer a vehicle with a driving cab inside
## 4. else the way the trainset faced: its head kept, a part split off facing as before, a scenery's
##    trainset from its first vehicle
## No trainset is made of vehicles off the tracks.

## A locomotive: a master controller of many positions and the front and rear cabs every test
## vehicle has
const ENGINE_PATH:String = "res://tests/fixtures/test_vehicle.fiz"
## A wagon: one controller position is none to turn, so no driving cab (FizTrainCntrlParser)
const WAGON_PATH:String = "res://tests/fixtures/test_wagon.fiz"
const COUPLER:int = RailVehicleController.COUPLING_FLAG_COUPLER
const FRONT:RailVehicleController.CouplerEnd = RailVehicleController.COUPLER_END_FRONT
const REAR:RailVehicleController.CouplerEnd = RailVehicleController.COUPLER_END_REAR
const NORMAL:TrackServer.Direction = TrackServer.DIRECTION_NORMAL
const REVERSED:TrackServer.Direction = TrackServer.DIRECTION_REVERSED
const COUPLED:RailVehicleController.CouplingMode = RailVehicleController.COUPLING_MODE_COUPLER
const UNCOUPLED:RailVehicleController.CouplingMode = RailVehicleController.COUPLING_MODE_NONE
const DRIVER:VehiclePersonRole.Role = VehiclePersonRole.VEHICLE_PERSON_ROLE_DRIVER
const TRACK_NAME:String = "TrainsetHeadTrack"
## A track long enough for every trainset here, and where a trainset's front stands on it [m]
const TRACK_LENGTH:float = 1000.0
const TRAINSET_OFFSET:float = 900.0

var _persons:Array[RID] = []
var _track:RID
var _trainset:RID


func before_each() -> void:
    _track = build_track(TRACK_NAME, TRACK_LENGTH)


func after_each() -> void:
    if _trainset.is_valid():
        RailVehicleServer.trainset_free(_trainset)
        _trainset = RID()
    TrackServer.track_free(_track)
    for person:RID in _persons:
        PersonServer.person_free(person)
    _persons.clear()


func _engine(vehicle_id:String) -> RID:
    return build_vehicle(vehicle_id, MaszynaFizManager.load_description_at(ENGINE_PATH)).get_rid()


func _wagon(vehicle_id:String) -> RID:
    return build_vehicle(vehicle_id, MaszynaFizManager.load_description_at(WAGON_PATH)).get_rid()


## The vehicles stood on the track one after another as a scenery's trainset: each [vehicle,
## direction, coupling to the next]; the pairs of no coupling are trainsets of their own
func _place(members:Array) -> void:
    await wait_idle_frames(2)
    _trainset = RailVehicleServer.trainset_create()
    RailVehicleServer.trainset_set_track(_trainset, _track, TRAINSET_OFFSET)
    for member:Array in members:
        RailVehicleServer.trainset_add_vehicle(_trainset, member[0], member[1], 0.0, member[2])
    RailVehicleServer.trainset_place(_trainset)


func _seat_driver(vehicle:RID, kind:RailVehicleCabinKind.Kind) -> void:
    var person:RID = PersonServer.person_create()
    _persons.append(person)
    assert_eq(VehicleServer.cabin_person_enter(RailVehicleServer.vehicle_get_cabin(vehicle, kind), person, DRIVER), OK)


func _uncouple(vehicle:RID, end:RailVehicleController.CouplerEnd) -> void:
    (VehicleServer.vehicle_get_controller(vehicle) as RailVehicleController).uncouple(end)


func _head(vehicle:RID) -> RID:
    return RailVehicleServer.trainset_get_head(RailVehicleServer.vehicle_get_trainset(vehicle))


func _vehicles(vehicle:RID) -> Array[RID]:
    var result:Array[RID] = []
    result.assign(RailVehicleServer.trainset_get_vehicles(RailVehicleServer.vehicle_get_trainset(vehicle)))
    return result


func test_an_engine_pulling_wagons_is_the_head() -> void:
    var engine:RID = _engine("HeadPullEngine")
    var wagon:RID = _wagon("HeadPullWagon")
    await _place([[engine, NORMAL, COUPLED], [wagon, NORMAL, UNCOUPLED]])
    assert_eq(_head(wagon), engine)
    assert_eq(_vehicles(wagon), [engine, wagon] as Array[RID], "from the head")
    assert_false(RailVehicleServer.vehicle_is_reversed_in_trainset(engine))
    assert_false(RailVehicleServer.vehicle_is_reversed_in_trainset(wagon))


func test_an_engine_pushing_is_the_head_too() -> void:
    var wagon:RID = _wagon("HeadPushWagon")
    var engine:RID = _engine("HeadPushEngine")
    await _place([[wagon, NORMAL, COUPLED], [engine, NORMAL, UNCOUPLED]])
    assert_eq(_head(wagon), engine, "whether it pulls or pushes")
    assert_eq(_vehicles(wagon), [engine, wagon] as Array[RID])
    assert_true(RailVehicleServer.vehicle_is_reversed_in_trainset(engine), "its rear is the trainset's front")
    assert_true(RailVehicleServer.vehicle_is_reversed_in_trainset(wagon))


func test_of_two_engines_at_one_end_the_outer_one_is_the_head() -> void:
    var outer:RID = _engine("HeadOuterEngine")
    var inner:RID = _engine("HeadInnerEngine")
    var wagon:RID = _wagon("HeadTwoEnginesWagon")
    await _place([[wagon, NORMAL, COUPLED], [inner, NORMAL, COUPLED], [outer, NORMAL, UNCOUPLED]])
    assert_eq(_head(wagon), outer)
    assert_eq(_vehicles(wagon), [outer, inner, wagon] as Array[RID])


func test_an_engine_inside_makes_the_nearer_end_the_head() -> void:
    var near:RID = _wagon("HeadNearWagon")
    var engine:RID = _engine("HeadInsideEngine")
    var far:RID = _wagon("HeadFarWagon")
    var farther:RID = _wagon("HeadFartherWagon")
    await _place([[farther, NORMAL, COUPLED], [far, NORMAL, COUPLED], [engine, NORMAL, COUPLED],
            [near, NORMAL, UNCOUPLED]])
    assert_eq(_head(far), near, "one wagon before the engine, two behind it")


func test_an_engine_inside_halfway_keeps_the_way_it_faced() -> void:
    var first:RID = _wagon("HeadHalfwayFirst")
    var engine:RID = _engine("HeadHalfwayEngine")
    var last:RID = _wagon("HeadHalfwayLast")
    await _place([[first, NORMAL, COUPLED], [engine, NORMAL, COUPLED], [last, NORMAL, UNCOUPLED]])
    assert_eq(_head(engine), first, "as the scenery set it")


func test_wagons_alone_keep_the_head_they_had() -> void:
    var first:RID = _wagon("HeadWagonsFirst")
    var second:RID = _wagon("HeadWagonsSecond")
    var third:RID = _wagon("HeadWagonsThird")
    await _place([[first, NORMAL, COUPLED], [second, NORMAL, UNCOUPLED], [third, NORMAL, UNCOUPLED]])
    assert_eq(_head(first), first, "the scenery's first vehicle")
    RailVehicleServer.coupler_attach(second, REAR, third, FRONT, COUPLER)
    assert_eq(_head(first), first, "a wagon coupled at the other end changes nothing")
    assert_eq(_vehicles(first), [first, second, third] as Array[RID])


func test_a_driver_at_the_other_end_makes_it_the_head() -> void:
    var front_engine:RID = _engine("HeadFrontEngine")
    var wagon:RID = _wagon("HeadDriverWagon")
    var rear_engine:RID = _engine("HeadRearEngine")
    await _place([[front_engine, NORMAL, COUPLED], [wagon, NORMAL, COUPLED], [rear_engine, REVERSED, UNCOUPLED]])
    assert_eq(_head(wagon), front_engine, "two engines, no driver: the scenery's first")
    assert_true(RailVehicleServer.vehicle_is_reversed_in_trainset(rear_engine))
    watch_signals(RailVehicleServer)

    _seat_driver(rear_engine, RailVehicleCabinKind.RAIL_VEHICLE_CABIN_FRONT)
    assert_eq(_head(wagon), rear_engine, "the end a driver sat down at")
    assert_signal_emitted_with_parameters(RailVehicleServer, "trainset_head_changed",
            [RailVehicleServer.vehicle_get_trainset(wagon)])
    assert_eq(_vehicles(wagon), [rear_engine, wagon, front_engine] as Array[RID], "from the new head")
    assert_false(RailVehicleServer.vehicle_is_reversed_in_trainset(rear_engine), "it faces the front now")
    assert_true(RailVehicleServer.vehicle_is_reversed_in_trainset(front_engine))

    _seat_driver(front_engine, RailVehicleCabinKind.RAIL_VEHICLE_CABIN_FRONT)
    assert_eq(_head(wagon), front_engine, "both driven: the latest")


func test_a_cab_switched_on_at_the_other_end_makes_it_the_head() -> void:
    var front_engine:RID = _engine("HeadActiveFront")
    var rear_engine:RID = _engine("HeadActiveRear")
    await _place([[front_engine, NORMAL, COUPLED], [rear_engine, REVERSED, UNCOUPLED]])
    _seat_driver(rear_engine, RailVehicleCabinKind.RAIL_VEHICLE_CABIN_FRONT)
    _seat_driver(front_engine, RailVehicleCabinKind.RAIL_VEHICLE_CABIN_FRONT)
    assert_eq(_head(front_engine), front_engine, "the latest driver's end")

    VehicleServer.vehicle_send_command(rear_engine, "cab_activation", true)
    assert_true((RailVehicleServer.vehicle_component_get(rear_engine,
            RailVehicleComponentType.COMPONENT_MASTER_CONTROLLER) as RailVehicleMasterController).is_cabin_master())
    assert_eq(_head(front_engine), rear_engine, "the cab switched on, in command")

    VehicleServer.vehicle_send_command(rear_engine, "cab_activation", false)
    VehicleServer.vehicle_send_command(front_engine, "cab_activation", true)
    assert_eq(_head(front_engine), front_engine, "switched over, the other end")


func test_coupling_two_trainsets_keeps_one_and_recomputes_it() -> void:
    var engine:RID = _engine("HeadMergeEngine")
    var wagon:RID = _wagon("HeadMergeWagon")
    var other_wagon:RID = _wagon("HeadMergeOtherWagon")
    var last_wagon:RID = _wagon("HeadMergeLastWagon")
    await _place([[engine, NORMAL, COUPLED], [wagon, NORMAL, UNCOUPLED], [other_wagon, NORMAL, COUPLED],
            [last_wagon, NORMAL, UNCOUPLED]])
    var engine_trainset:RID = RailVehicleServer.vehicle_get_trainset(engine)
    var wagons_trainset:RID = RailVehicleServer.vehicle_get_trainset(other_wagon)
    assert_ne(engine_trainset, wagons_trainset)
    watch_signals(RailVehicleServer)

    RailVehicleServer.coupler_attach(wagon, REAR, other_wagon, FRONT, COUPLER)
    assert_eq(RailVehicleServer.vehicle_get_trainset(last_wagon), engine_trainset, "the one with an engine stays")
    assert_signal_emitted_with_parameters(RailVehicleServer, "trainset_merged", [engine_trainset, wagons_trainset])
    assert_signal_emitted_with_parameters(RailVehicleServer, "trainset_freed", [wagons_trainset])
    assert_eq(_head(last_wagon), engine)
    assert_eq(_vehicles(last_wagon), [engine, wagon, other_wagon, last_wagon] as Array[RID])


func test_an_engine_coupled_to_wagons_becomes_their_head() -> void:
    var wagon:RID = _wagon("HeadJoinWagon")
    var last_wagon:RID = _wagon("HeadJoinLastWagon")
    var engine:RID = _engine("HeadJoinEngine")
    await _place([[wagon, NORMAL, COUPLED], [last_wagon, NORMAL, UNCOUPLED], [engine, REVERSED, UNCOUPLED]])
    RailVehicleServer.coupler_attach(last_wagon, REAR, engine, REAR, COUPLER)
    assert_eq(_head(wagon), engine, "at the wagons' tail, it heads them")
    assert_eq(_vehicles(wagon), [engine, last_wagon, wagon] as Array[RID])


func test_uncoupling_recomputes_both_parts() -> void:
    var engine:RID = _engine("HeadSplitEngine")
    var wagon:RID = _wagon("HeadSplitWagon")
    var other_engine:RID = _engine("HeadSplitOtherEngine")
    await _place([[engine, NORMAL, COUPLED], [wagon, NORMAL, COUPLED], [other_engine, REVERSED, UNCOUPLED]])
    _seat_driver(engine, RailVehicleCabinKind.RAIL_VEHICLE_CABIN_FRONT)
    assert_eq(_head(wagon), engine)
    watch_signals(RailVehicleServer)

    _uncouple(engine, REAR)
    assert_eq(_head(wagon), other_engine, "the engine left heads the part it stays in, driven or not")
    assert_eq(_head(engine), engine, "the one uncoupled heads itself")
    assert_signal_emit_count(RailVehicleServer, "trainset_vehicles_changed", 2, "both parts")


func test_a_part_split_off_keeps_facing_the_way_it_did() -> void:
    var first:RID = _wagon("HeadFacingFirst")
    var second:RID = _wagon("HeadFacingSecond")
    var third:RID = _wagon("HeadFacingThird")
    await _place([[first, NORMAL, COUPLED], [second, NORMAL, COUPLED], [third, REVERSED, UNCOUPLED]])
    _uncouple(first, REAR)
    assert_eq(_vehicles(second), [second, third] as Array[RID], "the same order as before")
    assert_true(RailVehicleServer.vehicle_is_reversed_in_trainset(third))


func test_a_scenerys_trainset_starts_from_its_first_vehicle() -> void:
    var first:RID = _wagon("HeadSceneryFirst")
    var second:RID = _wagon("HeadScenerySecond")
    await _place([[first, NORMAL, COUPLED], [second, REVERSED, UNCOUPLED]])
    assert_eq(RailVehicleServer.trainset_get_head(_trainset), first)
    assert_false(RailVehicleServer.vehicle_is_reversed_in_trainset(first))
    assert_true(RailVehicleServer.vehicle_is_reversed_in_trainset(second), "standing reversed in the scenery")


func test_doors_answer_by_the_side_towards_the_head() -> void:
    var engine:RID = _engine("HeadDoorsEngine")
    var wagon:RID = _wagon("HeadDoorsWagon")
    await _place([[engine, NORMAL, COUPLED], [wagon, REVERSED, UNCOUPLED]])
    var trainset:RID = RailVehicleServer.vehicle_get_trainset(wagon)
    assert_true(RailVehicleServer.vehicle_is_reversed_in_trainset(wagon))
    assert_false(RailVehicleServer.trainset_has_door_open(trainset, RailVehicleController.SIDE_LEFT))
    assert_false(RailVehicleServer.trainset_has_step_extended(trainset, RailVehicleController.SIDE_LEFT))
    assert_false(RailVehicleServer.trainset_has_door_permit(trainset, RailVehicleController.SIDE_RIGHT))


func test_vehicles_off_the_tracks_make_no_trainset_coupled_or_not() -> void:
    var engine:RID = _engine("HeadOffTrackEngine")
    var wagon:RID = _wagon("HeadOffTrackWagon")
    await wait_idle_frames(2)
    assert_eq(RailVehicleServer.vehicle_get_trainset(engine), RID(), "alone")
    RailVehicleServer.coupler_attach(engine, REAR, wagon, FRONT, COUPLER)
    assert_eq(RailVehicleServer.vehicle_get_trainset(engine), RID(), "coupled")
    assert_eq(RailVehicleServer.vehicle_get_trainset(wagon), RID())


func test_off_the_tracks_a_vehicle_has_no_side_of_a_trainset() -> void:
    var engine:RID = _engine("HeadNoTrainsetEngine")
    await wait_idle_frames(2)
    assert_false(RailVehicleServer.vehicle_is_reversed_in_trainset(engine))
    assert_engine_error("The vehicle is in no trainset.")


func test_no_trainset_has_no_head_nor_doors() -> void:
    var none:RID = RID()
    assert_eq(RailVehicleServer.trainset_get_head(none), RID())
    assert_engine_error("Parameter \"trainset\" is null")
    assert_false(RailVehicleServer.trainset_has_door_open(none, RailVehicleController.SIDE_LEFT))
    assert_engine_error("Parameter \"trainset\" is null")
    assert_false(RailVehicleServer.trainset_has_step_extended(none, RailVehicleController.SIDE_LEFT))
    assert_engine_error("Parameter \"trainset\" is null")
    assert_false(RailVehicleServer.trainset_has_door_permit(none, RailVehicleController.SIDE_LEFT))
    assert_engine_error("Parameter \"trainset\" is null")
