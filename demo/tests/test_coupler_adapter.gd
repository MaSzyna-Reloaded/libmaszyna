extends MaszynaGutTest

## A coupler adapter (TDynamicObject::attach_coupler_adapter(), remove_coupler_adapter(),
## DynObj.cpp:1754-1810): an end that is no automatic coupler, coupled to an automatic one, is fitted
## with the automatic one's adapter - the MMD's coupleradapter: or the original's own - and couples as
## an automatic one; taken off, the end is uncoupled first.

const ADAPTER_MMD:String = "res://tests/fixtures/dynamic/test/animated_v1/adapter.mmd"
## An EN57 cab car: automatic couplers at both ends
const AUTOMATIC_PATH:String = "res://tests/fixtures/dynamic/pkp/en57-2000_v1/6ba.fiz"
## A screw coupler
const SCREW_PATH:String = "res://tests/fixtures/dynamic/test/synthetic_v1/synthetic.fiz"
const TOLERANCE:float = 0.0001
## The original's own adapter (DynObj.cpp:1765-1769)
const ORIGINAL_ADAPTER_MODEL:String = "tabor/polsprzeg"
const ORIGINAL_ADAPTER_LENGTH:float = 0.085
## A straight track the two stand on, and where the trainset's front stands on it [m]
const TRACK_LENGTH:float = 100.0
const TRAINSET_OFFSET:float = 60.0
## The track's gauge [m]
const STANDARD_GAUGE:float = 1.435

var automatic:VehicleController
var screw:VehicleController
var track:RID
var trainset:RID


func before_each() -> void:
    automatic = build_vehicle("TestAdapterAutomatic", MaszynaFizManager.load_description_at(AUTOMATIC_PATH))
    screw = build_vehicle("TestAdapterScrew", MaszynaFizManager.load_description_at(SCREW_PATH))
    await wait_idle_frames(2)


func after_each() -> void:
    if trainset.is_valid():
        RailVehicleServer.trainset_free(trainset)
        trainset = RID()
    if track.is_valid():
        TrackServer.track_free(track)
        track = RID()


func test_the_mmd_adapter_is_read_as_the_data_writes_it() -> void:
    var adapter:Dictionary = MmdCabinInstancer.parse_coupler_adapter(
            ProjectSettings.globalize_path(ADAPTER_MMD), MmdCabinInstancer.vehicle_parameters("adapter", "adapter", ""))
    assert_eq(adapter["model"], "tabor/polsprzeg_schaku", "under models/, no extension")
    assert_almost_eq(adapter["length"], 0.085, TOLERANCE)
    assert_almost_eq(adapter["height"], 1.01, TOLERANCE)


func test_a_screw_end_takes_the_automatic_ones_adapter_and_gives_it_back() -> void:
    var front:RailVehicleController.CouplerEnd = RailVehicleController.COUPLER_END_FRONT
    assert_false(RailVehicleServer.coupler_is_automatic(screw.get_rid(), front))
    # the scenery couples the screw end to the automatic one: it is fitted with that one's adapter
    # (AttachNext(), DynObj.cpp:2740-2758)
    _place_screw_front_to_automatic_front()
    assert_true((screw as RailVehicleController).is_coupled(front))
    assert_true(RailVehicleServer.coupler_is_automatic(screw.get_rid(), front), "it couples as an automatic one")
    assert_eq(RailVehicleServer.coupler_get_adapter_model(screw.get_rid(), front),
            ORIGINAL_ADAPTER_MODEL, "the original's own: the automatic one's MMD has none")
    assert_almost_eq(RailVehicleServer.coupler_get_adapter_length(screw.get_rid(), front),
            ORIGINAL_ADAPTER_LENGTH, TOLERANCE)
    assert_true(RailVehicleServer.coupler_adapter_remove(screw.get_rid(), front))
    assert_eq(RailVehicleServer.coupler_get_adapter_model(screw.get_rid(), front), "")
    assert_false((screw as RailVehicleController).is_coupled(front), "uncoupled to take it off")


## The screw wagon ahead, reversed - its front towards the automatic wagon's front behind it
func _place_screw_front_to_automatic_front() -> void:
    track = TrackServer.track_create()
    var curve:TrackCurve = TrackCurve.new()
    curve.p1 = Vector3.ZERO
    curve.p2 = Vector3(TRACK_LENGTH, 0.0, 0.0)
    TrackServer.track_update_curves(track, curve, null)
    TrackServer.track_update(track, TrackServer.TRACK_NORMAL, "", STANDARD_GAUGE)
    TrackServer.topology_rebuild()
    trainset = RailVehicleServer.trainset_create()
    RailVehicleServer.trainset_set_track(trainset, track, TRAINSET_OFFSET)
    RailVehicleServer.trainset_add_vehicle(trainset, screw.get_rid(), TrackServer.DIRECTION_REVERSED, 0.0,
            RailVehicleController.COUPLING_MODE_COUPLER)
    RailVehicleServer.trainset_add_vehicle(trainset, automatic.get_rid(), TrackServer.DIRECTION_NORMAL, 0.0,
            RailVehicleController.COUPLING_MODE_NONE)
    RailVehicleServer.trainset_place(trainset)
