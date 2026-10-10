extends MaszynaGutTest

## A debug HUD section of a component the vehicle does not have says so instead of showing its
## controls - they kept the previous vehicle's values (reports#22: an EP07's spring brake section
## lit with the values of the vehicle shown before it).

const WITH_SPRING_BRAKE_PATH:String = "res://tests/fixtures/dynamic/pkp/wmb10_v1/wmb10.fiz"
const WITHOUT_SPRING_BRAKE_PATH:String = "res://tests/fixtures/dynamic/pkp/en57-2000_v1/6ba.fiz"
const SPRING_BRAKE_SECTION:PackedScene = preload("res://addons/libmaszyna/debug_hud/mover_switches_spring_brake.tscn")

var with_spring_brake:RID
var without_spring_brake:RID


func before_each() -> void:
    with_spring_brake = build_vehicle("TestHudSpringBrake",
            FizVehicleBuilder.build_description_at(WITH_SPRING_BRAKE_PATH)).get_rid()
    without_spring_brake = build_vehicle("TestHudNoSpringBrake",
            FizVehicleBuilder.build_description_at(WITHOUT_SPRING_BRAKE_PATH)).get_rid()
    await wait_idle_frames(2)


func test_a_section_without_its_component_says_not_applicable() -> void:
    var section:Control = SPRING_BRAKE_SECTION.instantiate()
    add_child_autofree(section)
    section.vehicle = with_spring_brake
    var not_applicable:Control = section.get_node("%NotApplicable")
    var lights:Control = section.get_node("Lights")
    assert_false(not_applicable.visible, "a vehicle with a spring brake")
    assert_true(lights.visible, "its lights shown")
    section.vehicle = without_spring_brake
    assert_true(not_applicable.visible, "a vehicle without a spring brake")
    assert_false(lights.visible, "no lights left from the previous vehicle")
