extends MaszynaGutTest

## A MaSzyna scenery lists its stations by its passenger stop indicators (W4): an event `putvalues
## <x> <y> <z> PassengerStopPoint:<name>#<indicator>` or a memory holding the command - one station
## of a name, in the middle of its indicators, its name as text with a space for each `_`.


func _stop_event(command:String, origin:Vector3, offset:Vector3) -> MaszynaEventData:
    var event:MaszynaEventData = MaszynaEventData.new()
    event.type = "putvalues"
    event.origin = origin
    event.parameters = PackedStringArray([str(offset.x), str(offset.y), str(offset.z), command, "1", "253"])
    return event


func _stop_memory(command:String, position:Vector3) -> MaszynaMemcellData:
    var memcell:MaszynaMemcellData = MaszynaMemcellData.new()
    memcell.text = command
    memcell.position = position
    return memcell


func test_a_station_of_each_stop_name_in_the_middle_of_its_indicators() -> void:
    var events:Array[MaszynaEventData] = [
        _stop_event("PassengerStopPoint:Elbląg#W4-P3-T1-2", Vector3(100.0, 0.0, 0.0), Vector3(10.0, 5.0, 20.0)),
        _stop_event("PassengerStopPoint:elbląg#W4-P3-T3-2", Vector3.ZERO, Vector3(130.0, 7.0, 40.0)),
        _stop_event("SetVelocity", Vector3.ZERO, Vector3(500.0, 0.0, 0.0)),
    ]
    var memcells:Array[MaszynaMemcellData] = [
        _stop_memory("PassengerStopPoint:Psie_Pole#7", Vector3(-50.0, 0.0, 300.0)),
        _stop_memory("Psie_Pole", Vector3(900.0, 0.0, 0.0)),
    ]

    var stations:Array[StationData] = MaszynaStations.stations_of_stop_points(events, memcells)

    assert_eq(stations.size(), 2, "one station of each stop name, the other commands left out")
    assert_eq(stations[0].station_name, "Elbląg", "the first name met, as text")
    assert_eq(stations[0].position, Vector3(120.0, 6.0, 30.0), "in the middle of its two indicators, each moved by its origin")
    assert_eq(stations[1].station_name, "Psie Pole", "a space for each `_`")
    assert_eq(stations[1].position, Vector3(-50.0, 0.0, 300.0), "where its one memory stands")


func test_a_compiled_scenery_holds_its_stations() -> void:
    var context:MaszynaImporterContext = MaszynaImporterContext.new()
    context.events.append(_stop_event("PassengerStopPoint:Kartuzy#W4-1", Vector3.ZERO, Vector3(1.0, 2.0, 3.0)))

    var compiled:MaszynaCompiledScenery = MaszynaSceneryProvider._compile_scenery("", "", context, [])

    assert_eq(compiled.stations.size(), 1)
    assert_eq((compiled.stations[0] as StationData).station_name, "Kartuzy")
