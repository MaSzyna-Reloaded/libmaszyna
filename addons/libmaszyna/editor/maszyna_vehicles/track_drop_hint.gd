@tool
extends Node

## While a vehicle is dragged over the 3D view: the named tracks near the cursor overlaid yellow, and
## green the one the cursor is unmistakably over - a drop there puts the vehicle on that track, at
## the offset under the cursor. Found through TrackServer's spatial index (tracks_find_in_aabb()).

## How far from the cursor a track is shown [m]
const NEAR_DISTANCE:float = 30.0
## How close to a track the cursor is over it [m] - about the track's own width
const HOVER_DISTANCE:float = 2.0
## How far apart the points a track's overlay is drawn through are [m]
const DRAW_STEP:float = 2.0
## How high over its curve a track's overlay is drawn, not to be hidden in its rails [m]
const DRAW_LIFT:float = 0.3
## How far the overlay reaches past the track's width on either side - over the sleepers [m]
const DRAW_MARGIN:float = 0.6
## The overlays, see-through
const NEAR_COLOR:Color = Color(Color.YELLOW, 0.6)
const HOVER_COLOR:Color = Color(Color.GREEN, 0.6)

var _mesh:ImmediateMesh = null
var _mesh_instance:MeshInstance3D = null
## The tracks drawn now, and the one of them hovered - drawn again only when either changes
var _near_tracks:Array[RID] = []
var _hovered_track:RID = RID()
var _hovered_offset:float = 0.0


## The name of the track the cursor is over, empty when it is over none
func get_hovered_track_name() -> String:
    return TrackServer.track_get_name(_hovered_track) if _hovered_track.is_valid() else ""


## Where along the hovered track the cursor is [m] - the vehicle's start_track_offset
func get_hovered_offset() -> float:
    return _hovered_offset


## The named tracks near the point, and the one it is over, are drawn
func show_tracks_near(world_position:Vector3) -> void:
    var cursor:Vector2 = Vector2(world_position.x, world_position.z)
    var near_tracks:Array[RID] = []
    var hovered:Array[RID] = []
    var hovered_offset:float = 0.0
    # the index is over (x, z)
    for track:RID in TrackServer.tracks_find_in_aabb(Rect2(cursor, Vector2.ZERO).grow(NEAR_DISTANCE)):
        var track_name:String = TrackServer.track_get_name(track)
        # a vehicle finds its start track by name - one another track carries too cannot be it
        if not track_name or not TrackServer.track_get_rid_by_name(track_name) == track:
            continue
        var curve:Curve3D = _get_curve(track)
        var offset:float = curve.get_closest_offset(world_position)
        var point:Vector3 = curve.sample_baked(offset)
        var distance:float = cursor.distance_to(Vector2(point.x, point.z))
        if distance > NEAR_DISTANCE:
            continue
        near_tracks.append(track)
        if distance <= HOVER_DISTANCE:
            hovered.append(track)
            hovered_offset = offset
    var hovered_track:RID = hovered[0] if hovered.size() == 1 else RID()
    _hovered_offset = hovered_offset
    if near_tracks == _near_tracks and hovered_track == _hovered_track:
        return
    _near_tracks = near_tracks
    _hovered_track = hovered_track
    if not _mesh_instance:
        var scene_root:Node = EditorInterface.get_edited_scene_root()
        if not scene_root:
            return
        var material:StandardMaterial3D = StandardMaterial3D.new()
        material.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
        material.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
        material.vertex_color_use_as_albedo = true
        material.cull_mode = BaseMaterial3D.CULL_DISABLED
        material.no_depth_test = true
        _mesh = ImmediateMesh.new()
        _mesh_instance = MeshInstance3D.new()
        _mesh_instance.mesh = _mesh
        _mesh_instance.material_override = material
        # the curves are in the world
        _mesh_instance.top_level = true
        scene_root.add_child(_mesh_instance, false, Node.INTERNAL_MODE_BACK)
    _mesh.clear_surfaces()
    # a band along each track as wide as the track and its sleepers
    for track:RID in _near_tracks:
        var curve:Curve3D = _get_curve(track)
        var length:float = curve.get_baked_length()
        var steps:int = maxi(ceili(length / DRAW_STEP), 1)
        var half_width:float = TrackServer.track_get_width(track) * 0.5 + DRAW_MARGIN
        var points:PackedVector3Array = PackedVector3Array()
        for step:int in steps + 1:
            points.append(curve.sample_baked(length * step / steps) + Vector3.UP * DRAW_LIFT)
        _mesh.surface_begin(Mesh.PRIMITIVE_TRIANGLE_STRIP)
        _mesh.surface_set_color(HOVER_COLOR if track == _hovered_track else NEAR_COLOR)
        for index:int in points.size():
            var along:Vector3 = points[mini(index + 1, steps)] - points[maxi(index - 1, 0)]
            var side:Vector3 = along.cross(Vector3.UP).normalized() * half_width
            _mesh.surface_add_vertex(points[index] - side)
            _mesh.surface_add_vertex(points[index] + side)
        _mesh.surface_end()


## Nothing is drawn or hovered any more
func clear() -> void:
    if _mesh_instance:
        _mesh_instance.queue_free()
    _mesh_instance = null
    _mesh = null
    _near_tracks.clear()
    _hovered_track = RID()
    _hovered_offset = 0.0


## A vehicle on a switch stands on its active track (RailVehicleServer.vehicle_set_track())
func _get_curve(track:RID) -> Curve3D:
    var branch:int = TrackServer.switch_get_active_track(track) if TrackServer.track_is_switch(track) \
            else TrackServer.TRACK_COMMON
    return TrackServer.track_get_domain_curve(track, branch)
