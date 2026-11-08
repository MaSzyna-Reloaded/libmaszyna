extends MaszynaGutTest

## What a trainset carries (RailVehicleServer.trainset_get_type()): recomputed by the server whenever
## its vehicles change, by the classifier it was handed. Its wagons - the vehicles without power - by
## their brake delays, as AutoRewident() counts them (Driver.cpp:2154-2175); the engine's own brake
## setting decides nothing.
## * the core's rule (RailVehicleTrainsetClassifier): a wagon with R, or without G, carries
##   passengers, one with G and without R goods, both kinds make a mixed trainset
## * the original's (MaszynaTrainsetClassifier, the plugin's): a passenger train while the goods wagons
##   are fewer than min(4, the others), else a goods train

const ENGINE_PATH:String = "res://tests/fixtures/test_vehicle.fiz"
const WAGON_PATH:String = "res://tests/fixtures/test_wagon.fiz"
const TRACK_NAME:String = "TrainsetTypeTrack"
## A track long enough for every trainset here, and where a trainset's front stands on it [m]
const TRACK_LENGTH:float = 1000.0
const TRAINSET_OFFSET:float = 900.0

var _track:RID
var _trainset:RID


func before_each() -> void:
    _track = build_track(TRACK_NAME, TRACK_LENGTH)


func after_each() -> void:
    RailVehicleServer.trainset_set_classifier(MaszynaTrainsetClassifier.new())
    if _trainset.is_valid():
        RailVehicleServer.trainset_free(_trainset)
        _trainset = RID()
    TrackServer.track_free(_track)


func _engine(vehicle_id:String) -> RID:
    return build_vehicle(vehicle_id, MaszynaFizManager.load_description_at(ENGINE_PATH)).get_rid()


## A wagon whose brake can take `delays` (RailVehicleBrake.BrakeDelaySetting)
func _wagon(vehicle_id:String, delays:int) -> RID:
    var wagon:RailVehicleController = build_vehicle(vehicle_id, MaszynaFizManager.load_description_at(WAGON_PATH)) \
            as RailVehicleController
    (wagon.get_rail_component(RailVehicleComponentType.COMPONENT_BRAKES) as RailVehicleBrake).cntrl_brake_delays = delays
    return wagon.get_rid()


## `count` wagons of `delays`
func _wagons(vehicle_id:String, count:int, delays:int) -> Array[RID]:
    var wagons:Array[RID] = []
    for index:int in count:
        wagons.append(_wagon("%s%d" % [vehicle_id, index], delays))
    return wagons


## The vehicles stood on the track one after another and coupled, as a scenery's trainset
func _place(vehicles:Array[RID]) -> void:
    await wait_idle_frames(2)
    _trainset = RailVehicleServer.trainset_create()
    RailVehicleServer.trainset_set_track(_trainset, _track, TRAINSET_OFFSET)
    for vehicle:RID in vehicles:
        RailVehicleServer.trainset_add_vehicle(_trainset, vehicle, TrackServer.DIRECTION_NORMAL, 0.0,
                RailVehicleController.COUPLING_MODE_COUPLER)
    RailVehicleServer.trainset_place(_trainset)


func _type_of(vehicle:RID) -> RailVehicleServer.TrainsetType:
    return RailVehicleServer.trainset_get_type(RailVehicleServer.vehicle_get_trainset(vehicle))


func test_coupling_makes_the_type_at_once() -> void:
    var engine:RID = _engine("TypeCouplingEngine")
    var wagon:RID = _wagon("TypeCouplingWagon", RailVehicleBrake.BRAKE_DELAY_GP)
    await _place([engine, wagon] as Array[RID])
    assert_eq(_type_of(engine), RailVehicleServer.TRAINSET_TYPE_CARGO, "no call of anybody's")
    assert_eq(_type_of(wagon), RailVehicleServer.TRAINSET_TYPE_CARGO, "one trainset, one type")


func test_uncoupling_makes_the_type_of_both_parts() -> void:
    var engine:RID = _engine("TypeSplitEngine")
    var wagon:RID = _wagon("TypeSplitWagon", RailVehicleBrake.BRAKE_DELAY_GP)
    await _place([engine, wagon] as Array[RID])
    (VehicleServer.vehicle_get_controller(engine) as RailVehicleController).uncouple(
            RailVehicleController.COUPLER_END_REAR)
    assert_eq(_type_of(engine), RailVehicleServer.TRAINSET_TYPE_NONE, "the engine alone: no wagon")
    assert_eq(_type_of(wagon), RailVehicleServer.TRAINSET_TYPE_CARGO, "the wagon split off is still goods")


func test_the_core_rule_takes_wagons_with_r_or_without_g_for_passenger_wagons() -> void:
    RailVehicleServer.trainset_set_classifier(RailVehicleTrainsetClassifier.new())
    var engine:RID = _engine("TypeCorePassengerEngine")
    var brake:RailVehicleBrake = RailVehicleServer.vehicle_component_get(
            engine, RailVehicleComponentType.COMPONENT_BRAKES) as RailVehicleBrake
    brake.cntrl_brake_delays = RailVehicleBrake.BRAKE_DELAY_G
    await _place([engine, _wagon("TypeCorePassengerWagon", RailVehicleBrake.BRAKE_DELAY_PR),
            _wagon("TypeCorePassengerWagon2", RailVehicleBrake.BRAKE_DELAY_P)] as Array[RID])
    assert_eq(_type_of(engine), RailVehicleServer.TRAINSET_TYPE_PASSENGER, "the engine at G decides nothing")


func test_the_core_rule_takes_goods_wagons_for_cargo() -> void:
    RailVehicleServer.trainset_set_classifier(RailVehicleTrainsetClassifier.new())
    var engine:RID = _engine("TypeCoreGoodsEngine")
    await _place([engine, _wagon("TypeCoreGoodsWagon", RailVehicleBrake.BRAKE_DELAY_GP)] as Array[RID])
    assert_eq(_type_of(engine), RailVehicleServer.TRAINSET_TYPE_CARGO)


func test_the_core_rule_takes_both_kinds_for_mixed() -> void:
    RailVehicleServer.trainset_set_classifier(RailVehicleTrainsetClassifier.new())
    var engine:RID = _engine("TypeCoreMixedEngine")
    await _place([engine, _wagon("TypeCoreMixedGoods", RailVehicleBrake.BRAKE_DELAY_GP),
            _wagon("TypeCoreMixedFast", RailVehicleBrake.BRAKE_DELAY_GPR)] as Array[RID])
    assert_eq(_type_of(engine), RailVehicleServer.TRAINSET_TYPE_MIXED)


func test_a_classifier_handed_over_retypes_every_trainset() -> void:
    var engine:RID = _engine("TypeHandedEngine")
    await _place([engine, _wagon("TypeHandedGoods", RailVehicleBrake.BRAKE_DELAY_GP),
            _wagon("TypeHandedFast", RailVehicleBrake.BRAKE_DELAY_GPR)] as Array[RID])
    assert_eq(_type_of(engine), RailVehicleServer.TRAINSET_TYPE_CARGO, "the original's: 1 goods is not < min(4, 1)")
    RailVehicleServer.trainset_set_classifier(RailVehicleTrainsetClassifier.new())
    assert_eq(_type_of(engine), RailVehicleServer.TRAINSET_TYPE_MIXED, "the core's at once")


func test_the_original_takes_a_few_goods_wagons_among_passenger_ones_for_a_passenger_train() -> void:
    var vehicles:Array[RID] = [_engine("TypeFewGoodsEngine")]
    vehicles.append_array(_wagons("TypeFewGoodsPassenger", 5, RailVehicleBrake.BRAKE_DELAY_PR))
    vehicles.append_array(_wagons("TypeFewGoodsGoods", 2, RailVehicleBrake.BRAKE_DELAY_GP))
    await _place(vehicles)
    assert_eq(_type_of(vehicles[0]), RailVehicleServer.TRAINSET_TYPE_PASSENGER, "2 goods < min(4, 5)")


func test_the_original_takes_four_goods_wagons_for_a_goods_train() -> void:
    var vehicles:Array[RID] = [_engine("TypeFourGoodsEngine")]
    vehicles.append_array(_wagons("TypeFourGoodsPassenger", 5, RailVehicleBrake.BRAKE_DELAY_PR))
    vehicles.append_array(_wagons("TypeFourGoodsGoods", 4, RailVehicleBrake.BRAKE_DELAY_GP))
    await _place(vehicles)
    assert_eq(_type_of(vehicles[0]), RailVehicleServer.TRAINSET_TYPE_CARGO, "4 goods is not < min(4, 5)")


func test_the_original_takes_more_goods_wagons_than_passenger_ones_for_a_goods_train() -> void:
    var vehicles:Array[RID] = [_engine("TypeMoreGoodsEngine")]
    vehicles.append_array(_wagons("TypeMoreGoodsPassenger", 3, RailVehicleBrake.BRAKE_DELAY_P))
    vehicles.append_array(_wagons("TypeMoreGoodsGoods", 3, RailVehicleBrake.BRAKE_DELAY_GP))
    await _place(vehicles)
    assert_eq(_type_of(vehicles[0]), RailVehicleServer.TRAINSET_TYPE_CARGO, "3 goods is not < min(4, 3)")


func test_a_trainset_that_is_no_more_has_no_type() -> void:
    assert_eq(RailVehicleServer.trainset_get_type(RID()), RailVehicleServer.TRAINSET_TYPE_NONE)
    assert_engine_error("Parameter \"trainset\" is null")
