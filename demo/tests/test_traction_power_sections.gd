extends MaszynaGutTest

## A traction node names a supply, and that supply is either a substation or a section of the
## network. The two are not the same thing: a substation feeds the span it is attached to, a
## section only says which part of the network the span belongs to, and the power has to reach it
## along the wires from a substation somewhere on that section (TTraction::PowerSet(),
## Traction.cpp:460). Every supply in this datapack is declared twice, once each way, under one
## name - so which of the two a span ends up on decides whether it carries anything.

const WIRE_HEIGHT:float = 5.8
const SPAN_LENGTH:float = 20.0
const NOMINAL_VOLTAGE:float = 3000.0
const SUBSTATION_VOLTAGE:float = 3600.0
## No load: VoltageGet's own "no current" case takes a 10 kOhm equivalent resistance.
const NO_CURRENT:float = 0.0

var _wires:Array[RID] = []
var _sources:Array[RID] = []


func after_each() -> void:
    for wire:RID in _wires:
        TractionServer.wire_free(wire)
    for source:RID in _sources:
        TractionServer.power_source_free(source)
    _wires.clear()
    _sources.clear()


func _add_source(name:String, voltage:float, is_section:bool) -> RID:
    var source:RID = TractionServer.power_source_create()
    _sources.append(source)
    TractionServer.power_source_set_params(
            source, name, voltage, 0.0, 0.2, 4500.0, 1.0, 3, 60.0, false, false, is_section)
    return source


func _add_wire(index:int, supply:String) -> RID:
    var wire:RID = TractionServer.wire_create()
    _wires.append(wire)
    TractionServer.wire_set_params(
            wire,
            Vector3(0.0, WIRE_HEIGHT, index * SPAN_LENGTH),
            Vector3(0.0, WIRE_HEIGHT, (index + 1) * SPAN_LENGTH),
            supply, NOMINAL_VOLTAGE, 2000.0, 0.01)
    return wire


## A span on a substation is fed directly, at the substation's voltage rather than its own
## declared one.
func test_a_span_on_a_substation_takes_the_substation_voltage() -> void:
    _add_source("pwr01", SUBSTATION_VOLTAGE, false)
    var wire:RID = _add_wire(0, "pwr01")
    TractionServer.network_build()

    assert_almost_eq(
            TractionServer.wire_get_voltage(wire, 0.0, NO_CURRENT), SUBSTATION_VOLTAGE, 1.0,
            "a directly powered span carries what the substation gives it")


## A span belonging to a section is fed through the wires, from the substation on that section.
func test_a_span_on_a_section_is_fed_along_the_wires() -> void:
    _add_source("sekcja", SUBSTATION_VOLTAGE, true)
    var fed:RID = _add_wire(0, "podstacja")
    _add_source("podstacja", SUBSTATION_VOLTAGE, false)
    var on_section:RID = _add_wire(1, "sekcja")
    TractionServer.network_build()

    assert_almost_eq(
            TractionServer.wire_get_voltage(fed, 0.0, NO_CURRENT), SUBSTATION_VOLTAGE, 1.0,
            "the span the substation sits on carries its voltage")
    assert_gt(
            TractionServer.wire_get_voltage(on_section, 0.0, NO_CURRENT), 0.0,
            "and the neighbouring span of the section is fed along the wire")


## The wire over a diverging track starts beside a span of the main line, not at its end, so no
## span of that chain is joined to a powered one. The original feeds it across the overlap from
## the nearest powered span of the same section (Traction.cpp:858-895) - td.scn's second track
## past zwr01 is wired exactly this way.
func test_a_chain_starting_beside_a_powered_span_is_fed_across_the_overlap() -> void:
    const BRANCH_OFFSET:float = 5.0
    const BRANCH_SPANS:int = 3
    const MAIN_SECTION_SPANS:int = 3
    ## The branch hangs from the middle of main span 2, half a metre off its wire.
    const BRANCH_START:Vector3 = Vector3(0.5, WIRE_HEIGHT, 2.5 * SPAN_LENGTH)
    _add_source("sekcja", SUBSTATION_VOLTAGE, true)
    _add_wire(0, "podstacja")
    _add_source("podstacja", SUBSTATION_VOLTAGE, false)
    for i in range(1, MAIN_SECTION_SPANS + 1):
        _add_wire(i, "sekcja")
    var branch_start:Vector3 = BRANCH_START
    var far:RID = RID()
    for i in BRANCH_SPANS:
        var branch_end:Vector3 = Vector3(BRANCH_OFFSET, WIRE_HEIGHT, BRANCH_START.z + (i + 1) * SPAN_LENGTH)
        far = TractionServer.wire_create()
        _wires.append(far)
        TractionServer.wire_set_params(far, branch_start, branch_end, "sekcja", NOMINAL_VOLTAGE, 2000.0, 0.01)
        branch_start = branch_end
    TractionServer.network_build()

    assert_almost_eq(
            TractionServer.wire_get_voltage(far, 0.0, NO_CURRENT), SUBSTATION_VOLTAGE, 1.0,
            "the far span of the branch carries the substation's voltage")


## Where the data declares one name twice - a section and then a substation, which is how every
## supply of this datapack is written - the last declaration is the one a span gets, the way the
## original's own name table resolves a duplicate (Names.h:38).
func test_the_last_declaration_of_a_name_is_the_one_a_span_gets() -> void:
    _add_source("pwr17", 3400.0, true)
    _add_source("pwr17", SUBSTATION_VOLTAGE, false)
    var wire:RID = _add_wire(0, "pwr17")
    TractionServer.network_build()

    assert_almost_eq(
            TractionServer.wire_get_voltage(wire, 0.0, NO_CURRENT), SUBSTATION_VOLTAGE, 1.0,
            "the substation declared last feeds the span, not the section declared before it")


## A span naming no supply at all keeps its own declared voltage - the insulator spans a scenery
## hangs between sections are written that way ("*").
func test_a_span_with_no_supply_keeps_its_own_voltage() -> void:
    var wire:RID = _add_wire(0, "*")
    TractionServer.network_build()

    assert_almost_eq(
            TractionServer.wire_get_voltage(wire, 0.0, NO_CURRENT), NOMINAL_VOLTAGE, 1.0,
            "no section and no supply means the span's own nominal voltage")


## A scenery declares resistivity in Ohm/km (Traction.cpp:112 turns it into Ohm/m). Read as Ohm/m
## a kilometre of wire had ten ohms, and a locomotive drawing a few hundred amps pulled the line
## under the line breaker's minimum voltage a few hundred metres from the substation.
func test_a_loaded_span_a_kilometre_from_the_substation_keeps_its_voltage() -> void:
    const SPANS_PER_KILOMETRE:int = 50
    const LOAD_CURRENT:float = 400.0
    const MIN_VOLTAGE_UNDER_LOAD:float = 3400.0
    _add_source("sekcja", SUBSTATION_VOLTAGE, true)
    _add_wire(0, "podstacja")
    _add_source("podstacja", SUBSTATION_VOLTAGE, false)
    var far:RID = RID()
    for i in range(1, SPANS_PER_KILOMETRE + 1):
        far = _add_wire(i, "sekcja")
    TractionServer.network_build()

    var voltage:float = SUBSTATION_VOLTAGE
    for i in 30:
        voltage = TractionServer.wire_get_voltage(far, voltage, LOAD_CURRENT)
        TractionServer.wire_draw_current(far, voltage, LOAD_CURRENT)
        await step(1)

    assert_gt(voltage, MIN_VOLTAGE_UNDER_LOAD, "a kilometre of wire should cost tens of volts, not the line")
