@tool
extends RefCounted

const WIRES_INT_TO_ENUM = {
    0: TractionRenderingServer.Wires.CONTACT,
    1: TractionRenderingServer.Wires.CONTACT_SUPPORT,
    2: TractionRenderingServer.Wires.TWO_CONTACT_ONE_SUPPORT,
    3: TractionRenderingServer.Wires.TWO_CONTACT_TWO_SUPPORT,
}

enum TractionMaterial {COPPER, ALUMINIUM}
const TractionData = preload("res://addons/libmaszyna/legacy/scenery/maszyna_traction_data.gd")


## Plain data holder - see maszyna_node_track_importer.gd's TrackData for why scenery-loaded
## traction is built directly against TractionRenderingServer's RID-based API
## (scenery_instancer.gd's _build_traction()) instead of Traction3D nodes.
func import(p:MaszynaParser, _context: MaszynaImporterContext) -> TractionData:
    var data := TractionData.new()
    data.power_supply_name = p.next_token()
    data.nominal_voltage = float(p.next_token())
    data.max_current = float(p.next_token())
    data.resistivity = float(p.next_token())
    data.material = (
        TractionMaterial.COPPER
        if p.next_token().to_lower() == "cu"
        else TractionMaterial.ALUMINIUM
    )
    data.wire_thickness = float(p.next_token())
    data.damage_flag = int(p.next_token())
    var contact_points = p.get_tokens(6).map(func(x): return float(x))
    data.contact_p1 = Vector3(contact_points[0], contact_points[1], contact_points[2])
    data.contact_p2 = Vector3(contact_points[3], contact_points[4], contact_points[5])
    var support_points = p.get_tokens(6).map(func(x): return float(x))
    data.support_p1 = Vector3(support_points[0], support_points[1], support_points[2])
    data.support_p2 = Vector3(support_points[3], support_points[4], support_points[5])
    data.min_height = float(p.next_token())
    data.segment_length = float(p.next_token())
    data.wires = WIRES_INT_TO_ENUM[int(p.next_token())]
    if not data.wires == TractionRenderingServer.Wires.CONTACT:
        data.wire_offset = float(p.next_token())
    data.visible = p.as_bool(p.next_token())
    var nt = p.next_token().to_lower()
    if nt == "parallel":
        data.parallel = p.next_token()
        nt = p.next_token()
    if not nt == "endtraction":
        push_error("Invalid node::traction")
    return data
