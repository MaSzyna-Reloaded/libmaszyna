@tool
extends RefCounted

const PowerSourceData = preload("res://addons/libmaszyna/legacy/scenery/maszyna_power_source_data.gd")

## Plain data holder - mirrors maszyna_node_traction_importer.gd's TractionData doc comment:
## built directly against TractionServer's RID-based API
## (scenery_instancer.gd's _build_power_source()) instead of a Node3D.
func import(p:MaszynaParser, context:MaszynaImporterContext) -> PowerSourceData:
    var data := PowerSourceData.new()
    var position:Vector3 = p.next_vector3()
    data.position = position.rotated(Vector3.UP, context.rotate.y) + context.origin
    data.nominal_voltage = float(p.next_token())
    data.voltage_frequency = float(p.next_token())
    data.internal_resistance = float(p.next_token())
    data.max_output_current = float(p.next_token())
    data.fast_fuse_timeout = float(p.next_token())
    data.fast_fuse_repetition = float(p.next_token())
    data.slow_fuse_timeout = float(p.next_token())
    if data.internal_resistance < 0.1:
        # matches TractionPower.cpp:65 - real-world DC substations are never this low
        data.internal_resistance = 0.2
    var token = p.next_token().to_lower()
    if token == "recuperation":
        data.recuperation = true
    elif token == "section":
        data.is_section = true
    while token and not token == "end":
        token = p.next_token()
    return data
