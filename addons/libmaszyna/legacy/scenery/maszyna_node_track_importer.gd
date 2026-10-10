@tool
extends RefCounted

## String spelling used by the .scn "track" node's environment token, in the same order as
## Track3D.TrackEnvironment (FLAT, BRIDGE, TUNNEL, MOUNTAINS, CANYON, BANK) - matches the
## original engine's own environment name list.
const ENVIRONMENT_NAMES:Array[String] = ["flat", "bridge", "tunnel", "mountains", "canyon", "bank"]
const TrackData = preload("res://addons/libmaszyna/legacy/scenery/maszyna_track_data.gd")


## Plain data holder - scenery-loaded tracks are built directly against TrackServer/
## TrackRenderingServer's RID-based API (see scenery_instancer.gd's _build_track()), not as
## TrackNormal3D/TrackSwitch3D nodes. A scenery can have thousands of these; a Node per segment
## (each with its own @tool script and _process()) is real, avoidable overhead that only actually
## benefits hand-authored scenes edited directly in demo_3d.tscn-style scenes.
func import(p:MaszynaParser, _context: MaszynaImporterContext) -> TrackData:
    var type_token = p.next_token()
    if type_token not in ["switch", "normal"]:
        # road/river/cross/turn/table are not built yet - discard the whole node, otherwise the
        # shared parser cursor stays inside it and keeps eating tokens until the next keyword.
        p.get_tokens_until("endtrack")
        return null

    var data := TrackData.new()
    data.type = TrackServer.TRACK_SWITCH if type_token == "switch" else TrackServer.TRACK_NORMAL

    data.length = float(p.next_token())
    data.width = float(p.next_token())
    data.friction = float(p.next_token())
    data.sound_distance = float(p.next_token())
    data.quality_flag = int(p.next_token())
    data.damage_flag = int(p.next_token())
    data.environment = _parse_environment(p.next_token())
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
    if type_token == "switch":
        data.diverging_curve = get_curve_from_tokens(p.get_tokens(15))
        nt = p.next_token()
    else:
        nt = p.next_token()

    if not nt == "endtrack":
        var params: Dictionary = {}
        var key: String = nt
        while key != "endtrack":
            var value: String = p.next_token()
            if value == "endtrack":
                break

            if key == "railprofile":
                data.railprofile = value
            elif MaszynaLegacyEventFactory.TRACK_EVENTS.has(key):
                # a slot's names are gathered: a second `event2` is another event, not a replacement
                var names:PackedStringArray = data.events.get(key, PackedStringArray())
                names.append(value)
                data.events[key] = names
            else:
                params[key] = value

            key = p.next_token()
        data.parameters = params
    return data


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


func _parse_environment(token:String) -> int:
    var index:int = ENVIRONMENT_NAMES.find(token.to_lower())
    return index if index >= 0 else 0
