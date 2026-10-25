@tool
extends RefCounted

## The .scn "track" node's environment tokens (Track.cpp:461-477); any other is unknown
const ENVIRONMENTS:Dictionary[String, MaszynaTrackData.MaszynaEnvironmentType] = {
    "flat": MaszynaTrackData.MaszynaEnvironmentType.FLAT,
    "mountains": MaszynaTrackData.MaszynaEnvironmentType.MOUNTAINS,
    "mountain": MaszynaTrackData.MaszynaEnvironmentType.MOUNTAINS,
    "canyon": MaszynaTrackData.MaszynaEnvironmentType.CANYON,
    "tunnel": MaszynaTrackData.MaszynaEnvironmentType.TUNNEL,
    "bridge": MaszynaTrackData.MaszynaEnvironmentType.BRIDGE,
    "bank": MaszynaTrackData.MaszynaEnvironmentType.BANK,
}
## The environments that are a feature of the segment itself (TrackData.structure)
const STRUCTURES:Dictionary[MaszynaTrackData.MaszynaEnvironmentType, TrackData.Structure] = {
    MaszynaTrackData.MaszynaEnvironmentType.BRIDGE: TrackData.STRUCTURE_BRIDGE,
    MaszynaTrackData.MaszynaEnvironmentType.TUNNEL: TrackData.STRUCTURE_TUNNEL,
}
## What a track's textures say it is - its bed, a road's surface and centre line - by the textures'
## names (TrackData's features; the .scn knows only textures)
const FEATURES:MaszynaTrackFeatures = preload("maszyna_track_features.tres")
## The track types built so far; a road is read as a normal track (Track.cpp:397-438: road is
## tt_Normal with the road category), a crossroad of roads as its two curves (tt_Cross, Track.cpp:422-426)
const TYPES:Dictionary[String, TrackServer.TrackType] = {
    "normal": TrackServer.TRACK_NORMAL,
    "switch": TrackServer.TRACK_SWITCH,
    "road": TrackServer.TRACK_ROAD,
    "cross": TrackServer.TRACK_CROSS,
}
## The types with a second curve: a switch's diverging one, a crossroad's second road
const TWO_CURVE_TYPES:Array[String] = ["switch", "cross"]


## Plain data holder - scenery-loaded tracks are built directly against TrackServer/
## TrackRenderingServer's RID-based API (see SceneryServer.build_track()), not as
## TrackNormal3D/TrackSwitch3D nodes. A scenery can have thousands of these; a Node per segment
## (each with its own @tool script and _process()) is real, avoidable overhead that only actually
## benefits hand-authored scenes edited directly in demo_3d.tscn-style scenes.
func import(p:MaszynaParser, _context: MaszynaImporterContext) -> MaszynaTrackData:
    var type_token = p.next_token()
    if not TYPES.has(type_token):
        # river/turn/table are not built yet - discard the whole node, otherwise the
        # shared parser cursor stays inside it and keeps eating tokens until the next keyword.
        p.get_tokens_until("endtrack")
        return null

    var data := MaszynaTrackData.new()
    data.kind = TYPES[type_token]

    data.length = float(p.next_token())
    data.width = float(p.next_token())
    data.friction = float(p.next_token())
    data.joint_spacing = float(p.next_token())
    data.meta[MaszynaTrackData.META_QUALITY] = int(p.next_token())
    data.damage = int(p.next_token())
    var environment:MaszynaTrackData.MaszynaEnvironmentType = _parse_environment(p.next_token())
    data.meta[MaszynaTrackData.META_ENVIRONMENT] = environment
    data.structure = STRUCTURES.get(environment, TrackData.STRUCTURE_NONE)
    data.visible = p.as_bool(p.next_token())
    if data.visible:
        data.material1 = _parse_material(p.next_token())
        data.tex_length = float(p.next_token())
        data.material2 = _parse_material(p.next_token())
        data.tex_height = float(p.next_token())
        data.tex_width = float(p.next_token())
        data.tex_slope = float(p.next_token())
    data.curve = get_curve_from_tokens(p.get_tokens(15))

    var nt = ""
    # the keywords in lower case, as the original reads every token (Track.cpp:810-928)
    if type_token in TWO_CURVE_TYPES:
        data.diverging_curve = get_curve_from_tokens(p.get_tokens(15))
        nt = p.next_token().to_lower()
    else:
        nt = p.next_token().to_lower()

    if not nt == "endtrack":
        var params: Dictionary = {}
        var key: String = nt
        while key != "endtrack":
            var value: String = p.next_token()
            if value == "endtrack":
                break

            if key == "railprofile":
                data.railprofile = value
            elif MaszynaEventFactory.TRACK_EVENTS.has(key):
                # a slot's names are gathered: a second `event2` is another event, not a replacement
                var names:PackedStringArray = data.events.get(key, PackedStringArray())
                names.append(value)
                data.events[key] = names
            else:
                params[key] = value

            key = p.next_token().to_lower()
        # the speed limit a `trackvel` event changes (Track.cpp:851-858)
        data.speed_limit = float(params.get("velocity", -1.0))
        params.erase("velocity")
        data.parameters = params
    # a crossroad is drawn otherwise than a track (Track.cpp:1519), which is not ported: it stays
    # undrawn, as before it was read at all - its roads are RoadServer's (MaszynaRoadNetwork)
    if data.kind == TrackServer.TRACK_CROSS:
        data.visible = false
    _read_features(data)
    return data


## The features of TrackData the .scn gives only as textures: a rail track's bed from its second
## texture, a road's surface and centre line from its first, its shoulder from having a second
func _read_features(data:MaszynaTrackData) -> void:
    if data.kind == TrackServer.TRACK_ROAD:
        for rule:MaszynaRoadSurfaceRule in FEATURES.road_rules:
            if data.material1.match(rule.material):
                data.surface = rule.surface
                data.centre_line = rule.centre_line
                break
        data.shoulder = not data.material2 == ""
        return
    for rule:MaszynaTrackBedRule in FEATURES.bed_rules:
        if data.material2.match(rule.material):
            data.bed = rule.bed
            break


func get_curve_from_tokens(points) -> TrackCurve:
    var c = TrackCurve.new()
    c.p1 = Vector3(float(points[0]), float(points[1]), float(points[2]))
    c.c1 = Vector3(float(points[4]), float(points[5]), float(points[6]))
    c.c2 = Vector3(float(points[7]), float(points[8]), float(points[9]))
    c.p2 = Vector3(float(points[10]), float(points[11]), float(points[12]))
    c.roll1 = float(points[3])
    c.roll2 = float(points[13])
    c.radius = float(points[14])

    return c


## "none" is the format's sentinel for "no texture here", not a material name - the original
## engine stores a null handle for it and then draws nothing (Track.cpp:485-491). Kept as an
## empty name so the trackbed is skipped instead of being painted with the missing-texture
## placeholder.
func _parse_material(token:String) -> String:
    var name:String = token.to_lower()
    return "" if name == "none" else name


func _parse_environment(token:String) -> MaszynaTrackData.MaszynaEnvironmentType:
    return ENVIRONMENTS.get(token.to_lower(), MaszynaTrackData.MaszynaEnvironmentType.UNKNOWN)
