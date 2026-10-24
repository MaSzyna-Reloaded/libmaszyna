@tool
extends RefCounted

## The widest line the original draws (scenenode.cpp:582)
const MAX_WIDTH:float = 30.0
## The node's colour is given 0-255 (scenenode.cpp:577)
const COLOR_SCALE:float = 255.0
const END_TOKEN:String = "endline"


## lines_node::import() (scenenode.cpp:561-649): `r g b width x y z ... endline`, the points of a
## "line_strip" or a "line_loop" turned into pairs of lines, placed by the enclosing rotate and
## origin as basic_region::insert() places them (scene.cpp:1370-1390); null for a node of no line
func import(p:MaszynaParser, context:MaszynaImporterContext, type:String, range_min:float, range_max:float) -> LinesData:
    var color:Color = Color(
        float(p.next_token()) / COLOR_SCALE, float(p.next_token()) / COLOR_SCALE, float(p.next_token()) / COLOR_SCALE
    )
    var width:float = minf(MAX_WIDTH, float(p.next_token()))
    var basis:Basis = Basis.from_euler(context.rotate)
    var read:PackedVector3Array = []
    var token:String = p.next_token()
    while token and not token == END_TOKEN:
        read.append(basis * Vector3(float(token), float(p.next_token()), float(p.next_token())) + context.origin)
        token = p.next_token()

    var points:PackedVector3Array = []
    if type == "lines":
        points = read
    else:
        for index:int in range(1, read.size()):
            points.append(read[index - 1])
            points.append(read[index])
        if type == "line_loop" and read.size() > 2:
            points.append(read[read.size() - 1])
            points.append(read[0])
    if points.size() % 2:
        push_error("Lines node specified odd number of vertices")
        points.resize(points.size() - 1)
    if not points:
        return null

    var lines:LinesData = LinesData.new()
    lines.points = points
    lines.color = color
    lines.width = width
    lines.range_min = range_min
    lines.range_max = range_max
    return lines
