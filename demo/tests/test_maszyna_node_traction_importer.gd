extends MaszynaGutTest

## maszyna_node_traction_importer.gd - the ".scn traction node" reader. The power source and the
## parallel span are named in lower case, as the nodes they name are (Traction.cpp:101-103, 147-148;
## simulationstateserializer.cpp:425-430) - "PWR01" would leave the wire without its power source

const TractionImporter = preload("res://addons/libmaszyna/legacy/scenery/maszyna_node_traction_importer.gd")

## ep07.scn's first span, its power source and a parallel span named with capitals
const SOURCE:String = """
PWR01 3500 4500 0.01 cu 3.0 1
10.1077 5.845 -72.2173
2.1656 5.845 -11.8011
10.1077 7.545 -72.2173
2.1656 7.545 -11.8011
0.4 4.0 3 0.04 vis
parallel Przeslo_2
endtraction
"""


func test_the_names_with_capitals_are_read_in_lower_case() -> void:
    var parser:MaszynaParser = MaszynaParser.new()
    parser.initialize(SOURCE.to_utf8_buffer())
    var traction:MaszynaTractionData = TractionImporter.new().import(parser, MaszynaImporterContext.new())

    assert_eq(traction.power_supply_name, "pwr01", "the power source")
    assert_eq(traction.parallel, "przeslo_2", "the parallel span")
