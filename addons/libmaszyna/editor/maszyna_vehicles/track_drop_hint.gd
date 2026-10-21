@tool
extends Node

## While a vehicle is dragged over the 3D view: the named tracks of its kind (group) near the
## cursor's ray overlaid yellow, and green the one the ray passes nearest - a drop there puts the
## vehicle on that track, at the offset under the cursor. Picked by the ray itself, not by the point
## it lands on: the ground of a scenery has no collision in the editor, so that point is on the
## plane y = 0 and slides with the camera's angle wherever the ground is not at 0. Found through
## TrackServer's spatial index (tracks_find_in_aabb()).

## How far from the cursor's ray a track is shown [m]
const NEAR_DISTANCE:float = 30.0
## How close to a track the cursor's ray passes when it is over it [m] - about the track's own width
const HOVER_DISTANCE:float = 2.0
## How far along the cursor's ray tracks are looked for [m]
const RAY_REACH:float = 2000.0
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
## The kind of track the dragged vehicle runs on (set_vehicle_category())
var _group:TrackServer.TrackTypeGroup = TrackServer.GROUP_RAIL


## The dragged vehicle's category picks the tracks shown: roads for a road vehicle, else rails
func set_vehicle_category(category:VehicleInfo.VehicleCategory) -> void:
    _group = TrackServer.GROUP_ROAD if category in VehicleInfo.ROAD_CATEGORIES else TrackServer.GROUP_RAIL


## The name of the track the cursor is over, empty when it is over none
func get_hovered_track_name() -> String:
    return TrackServer.track_get_name(_hovered_track) if _hovered_track.is_valid() else ""


## Where along the hovered track the cursor is [m] - the vehicle's start_track_offset
func get_hovered_offset() -> float:
    return _hovered_offset


## The named tracks of the vehicle's group near the cursor's ray, and the one it passes nearest, are
## drawn ([param _world_position] is where the dragged preview stands)
func show_tracks_near(_world_position:Vector3, ray_origin:Vector3, ray_direction:Vector3) -> void:
    var ray_end:Vector3 = ray_origin + ray_direction * RAY_REACH
    var near_tracks:Array[RID] = []
    var hovered_track:RID = RID()
    var hovered_distance:float = HOVER_DISTANCE
    var hovered_offset:float = 0.0
    # the index is over (x, z): the ray's footprint
    var footprint:Rect2 = Rect2(Vector2(ray_origin.x, ray_origin.z), Vector2.ZERO).expand(
            Vector2(ray_end.x, ray_end.z)).grow(NEAR_DISTANCE)
    for track:RID in TrackServer.tracks_find_in_aabb(footprint):
        if not TrackServer.track_get_group(track) == _group:
            continue
        var track_name:String = TrackServer.track_get_name(track)
        # a vehicle finds its start track by name - one another track carries too cannot be it
        if not track_name or not TrackServer.track_get_rid_by_name(track_name) == track:
            continue
        var curve:Curve3D = _get_curve(track)
        var length:float = curve.get_baked_length()
        var steps:int = maxi(ceili(length / DRAW_STEP), 1)
        # the track's point nearest to the ray, and how far from it the ray passes
        var nearest:float = INF
        var nearest_point:Vector3 = Vector3.ZERO
        for step:int in steps + 1:
            var point:Vector3 = curve.sample_baked(length * step / steps)
            var along:float = clampf((point - ray_origin).dot(ray_direction), 0.0, RAY_REACH)
            var distance:float = point.distance_to(ray_origin + ray_direction * along)
            if distance < nearest:
                nearest = distance
                nearest_point = point
        if nearest > NEAR_DISTANCE:
            continue
        near_tracks.append(track)
        if nearest <= hovered_distance:
            hovered_track = track
            hovered_distance = nearest
            # the place on the track under the ray: the ray's point nearest to the track's
            var along_ray:float = maxf((nearest_point - ray_origin).dot(ray_direction), 0.0)
            hovered_offset = curve.get_closest_offset(ray_origin + ray_direction * along_ray)
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
    var branch:TrackServer.SwitchTrack = (
            TrackServer.switch_get_active_track(TrackServer.track_get_switch(track))
            if TrackServer.track_is_switch(track) else TrackServer.TRACK_COMMON)
    return TrackServer.track_get_domain_curve(track, branch)
