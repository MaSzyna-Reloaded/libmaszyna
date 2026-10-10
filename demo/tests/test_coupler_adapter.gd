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

var automatic:VehicleController
var screw:VehicleController


func before_each() -> void:
    automatic = build_vehicle("TestAdapterAutomatic", FizVehicleBuilder.build_description_at(AUTOMATIC_PATH))
    screw = build_vehicle("TestAdapterScrew", FizVehicleBuilder.build_description_at(SCREW_PATH))
    await wait_idle_frames(2)


func test_the_mmd_adapter_is_read_as_the_data_writes_it() -> void:
    var adapter:Dictionary = MmdCabinInstancer.parse_coupler_adapter(
            ProjectSettings.globalize_path(ADAPTER_MMD), MmdCabinInstancer.vehicle_parameters("adapter", "adapter", ""))
    assert_eq(adapter["model"], "tabor/polsprzeg_schaku", "under models/, no extension")
    assert_almost_eq(adapter["length"], 0.085, TOLERANCE)
    assert_almost_eq(adapter["height"], 1.01, TOLERANCE)


func test_a_screw_end_takes_the_automatic_ones_adapter_and_gives_it_back() -> void:
    var front:RailVehicleController.CouplerEnd = RailVehicleController.COUPLER_END_FRONT
    assert_false(RailVehicleServer.vehicle_is_coupler_automatic(screw.get_rid(), front))
    (screw as RailVehicleController).couple(automatic, front, front, RailVehicleController.COUPLING_FLAG_COUPLER)
    assert_true((screw as RailVehicleController).coupler_adapter_fit(front))
    assert_true(RailVehicleServer.vehicle_is_coupler_automatic(screw.get_rid(), front), "it couples as an automatic one")
    assert_eq(RailVehicleServer.vehicle_get_coupler_adapter_model(screw.get_rid(), front),
            ORIGINAL_ADAPTER_MODEL, "the original's own: the automatic one's MMD has none")
    assert_almost_eq(RailVehicleServer.vehicle_get_coupler_adapter_length(screw.get_rid(), front),
            ORIGINAL_ADAPTER_LENGTH, TOLERANCE)
    assert_true((screw as RailVehicleController).coupler_adapter_remove(front))
    assert_eq(RailVehicleServer.vehicle_get_coupler_adapter_model(screw.get_rid(), front), "")
    assert_false((screw as RailVehicleController).is_coupled(front), "uncoupled to take it off")
