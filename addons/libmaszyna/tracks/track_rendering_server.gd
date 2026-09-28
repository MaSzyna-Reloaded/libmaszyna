@tool
extends Node


const UNKNOWN_MATERIAL = preload("res://addons/libmaszyna/legacy/materials/unknown.tres")
const _SWITCH_SEGMENT_COUNT: int = 6
const _SWITCH_BLADE_RATIO: float = 0.65
const _MIN_CURVED_MESH_SEGMENT_LENGTH: float = 2.0
const _MAX_MESH_SEGMENT_LENGTH: float = 10.0


class TrackState:
    var track_rid: RID = RID()
    ## Valid when the track is streamed (SceneryStreamingServer); hand-placed Track3D nodes are not
    var stream_rid: RID = RID()
    var streamed: bool = false
    var primary_rail_mesh_instance: RID = RID()
    var primary_rail_mesh: Mesh
    var secondary_rail_mesh_instance: RID = RID()
    var secondary_rail_mesh: Mesh
    var primary_blade_mesh_instance: RID = RID()
    var primary_blade_mesh: Mesh
    var primary_blade_move_direction: Vector3 = Vector3.ZERO
    var primary_blade_offset: float = 0.0
    var primary_blade_pivot: Vector3 = Vector3.ZERO
    var primary_blade_angle_factor: float = 0.0
    var secondary_blade_mesh_instance: RID = RID()
    var secondary_blade_mesh: Mesh
    var secondary_blade_move_direction: Vector3 = Vector3.ZERO
    var secondary_blade_offset: float = 0.0
    var secondary_blade_pivot: Vector3 = Vector3.ZERO
    var secondary_blade_angle_factor: float = 0.0
    var switch_is_right: bool = false
    var trackbed_mesh_instance: RID = RID()
    var trackbed_mesh: Mesh
    var tex_length: float = 4.0
    var tex_height: float = 0.0
    var tex_width: float = 0.0
    var tex_slope: float = 0.0
    var railprofile: String = "default"
    var material1_name: String = ""
    var material2_name: String = ""
    var material_trackbed_name: String = ""
    var material1: Material
    var material2: Material
    var material_trackbed: Material
    ## Trackbed material after the neighbour walk, cached until the topology changes.
    var resolved_trackbed_material: Material
    ## Length the trackbed texture covers - its material's size, else the track's own tex_length
    ## (TTrack::texture_length(), Track.cpp:2481-2493); resolved with the material
    var resolved_trackbed_texture_length: float = 4.0
    var resolved_trackbed_material_valid: bool = false
    var rail_visible: bool = true
    var ballast_visible: bool = true


class TrackData:
    var track_rid: RID = RID()
    var curve1: TrackCurve
    var curve2: TrackCurve
    var width: float = 0.0
    var switch_common_endpoint_index: int = TrackServer.POINT_NONE
    var switch_f_offset1: float = 0.0
    var switch_f_offset2: float = 0.0
    var endpoints: PackedVector3Array = PackedVector3Array()

    func is_valid() -> bool:
        return track_rid.is_valid() and curve1 != null


class RailProfile:
    var rail: Array
    var blade: Array

    func _init(rail:Array, blade:Array = []):
        self.rail = rail
        self.blade = rail if not blade else blade


const DEFAULT_RAIL_PROFILE_POINTS: Array = [
    [0.111, -0.180, 1.000, 0.000, 0.00],
    [0.046, -0.150, 0.707, 0.707, 0.15],
    [0.044, -0.050, 0.707, -0.707, 0.25],
    [0.073, -0.038, 0.707, -0.707, 0.35],
    [0.072, -0.010, 0.707, 0.707, 0.40],
    [0.052, -0.000, 0.000, 1.000, 0.45],
    [0.020, -0.000, 0.000, 1.000, 0.55],
    [0.000, -0.010, -0.707, 0.707, 0.60],
    [-0.001, -0.038, -0.707, -0.707, 0.65],
    [0.028, -0.050, -0.707, -0.707, 0.75],
    [0.026, -0.150, -0.707, 0.707, 0.85],
    [-0.039, -0.180, -1.000, 0.000, 1.00],
]

var DEFAULT_RAIL_PROFILE:RailProfile = RailProfile.new(
        DEFAULT_RAIL_PROFILE_POINTS,
        DEFAULT_RAIL_PROFILE_POINTS,
    )

var _tracks: Dictionary[RID, TrackState] = {}
var _track_render_rids_by_track: Dictionary[RID, RID] = {}
var _next_track_render_id: int = 0
var _stream_owner: int = -1
var _rail_profile_cache: Dictionary = {}
## Texture length ("size:" y) by material name, -1 for none - parsed once per material
var _material_texture_lengths: Dictionary[String, float] = {}
var _rail_profile_regex: RegEx = RegEx.new()
## Created in _ready(): this autoload is instantiated before MaterialManager
var _track_material_options: MaterialManager.MaterialOptions
const _SWITCH_TRACKBED_Z_FIGHT_OFFSET: float = 0.025
const _SWITCH_BLADE_OFFSET: float = 0.1
const _TRACKBED_SECTION_POINT_COUNT: int = 5
## Original engine: Track.cpp:2855 - a trackbed texture 4 m long keeps the old fixed mapping
const _TRACKBED_OLD_MAPPING_TEXTURE_LENGTH: float = 4.0
## Original engine: Track.cpp:2857-2858 - the old mapping's slope normals, 75 degrees up
const _TRACKBED_OLD_MAPPING_NORMAL_ANGLE: float = 75.0
## Original engine: Track.cpp:2869, 2877 - the old mapping's u at the slope's upper edges
const _TRACKBED_OLD_MAPPING_BREAK_U: float = 0.33

func _init() -> void:
    _rail_profile_regex.compile("-?\\d+(?:\\.\\d+)?")


func _ready() -> void:
    _track_material_options = MaterialManager.MaterialOptions.new()
    _track_material_options.cull_disabled = true
    TrackServer.topology_rebuilt.connect(_on_topology_rebuilt)
    TrackServer.switch_offset_changed.connect(_on_switch_offset_changed)
    GameDataServer.data_unload_requested.connect(_on_data_unload_requested)
    GameDataServer.data_reload_requested.connect(_on_data_reload_requested)


## The rail profiles and the materials' sizes are the old data's
func _on_data_unload_requested() -> void:
    _rail_profile_cache.clear()
    _material_texture_lengths.clear()


## Every track is built again, taking its materials again: a streamed one as it is streamed, one
## built directly at once
func _on_data_reload_requested() -> void:
    for track_render_rid: RID in _tracks:
        var state: TrackState = _tracks[track_render_rid]
        if not state.stream_rid.is_valid():
            rebuild_track(track_render_rid)
    if _stream_owner >= 0:
        SceneryStreamingServer.owner_rebuild(_stream_owner)


func create_track(track_rid: RID) -> RID:
    var state: TrackState = TrackState.new()
    state.track_rid = track_rid
    state.primary_rail_mesh_instance = RenderingServer.instance_create()
    state.secondary_rail_mesh_instance = RenderingServer.instance_create()
    state.primary_blade_mesh_instance = RenderingServer.instance_create()
    state.secondary_blade_mesh_instance = RenderingServer.instance_create()
    state.trackbed_mesh_instance = RenderingServer.instance_create()
    # The trackbed casts no sun shadow: a 0.2 m bed barely shades the ground, but its slopes, lit
    # at a grazing angle, shaded themselves - a dark band that read as a gap to the terrain
    # (FINDINGS.md, 2026-09-28)
    RenderingServer.instance_geometry_set_cast_shadows_setting(
        state.trackbed_mesh_instance, RenderingServer.SHADOW_CASTING_SETTING_OFF
    )

    _next_track_render_id += 1
    var track_render_rid: RID = rid_from_int64(_next_track_render_id)
    _tracks[track_render_rid] = state
    _track_render_rids_by_track[track_rid] = track_render_rid
    return track_render_rid


func free_track(track_render_rid: RID) -> void:
    var state: TrackState = _tracks.get(track_render_rid)
    if not state:
        return

    if state.stream_rid.is_valid():
        SceneryStreamingServer.stream_free(state.stream_rid)
    _reset_track_instances(state)

    if state.primary_rail_mesh_instance.is_valid():
        RenderingServer.free_rid(state.primary_rail_mesh_instance)
    if state.secondary_rail_mesh_instance.is_valid():
        RenderingServer.free_rid(state.secondary_rail_mesh_instance)
    if state.primary_blade_mesh_instance.is_valid():
        RenderingServer.free_rid(state.primary_blade_mesh_instance)
    if state.secondary_blade_mesh_instance.is_valid():
        RenderingServer.free_rid(state.secondary_blade_mesh_instance)
    if state.trackbed_mesh_instance.is_valid():
        RenderingServer.free_rid(state.trackbed_mesh_instance)

    state.primary_rail_mesh = null
    state.secondary_rail_mesh = null
    state.primary_blade_mesh = null
    state.secondary_blade_mesh = null
    state.trackbed_mesh = null

    if _track_render_rids_by_track.get(state.track_rid) == track_render_rid:
        _track_render_rids_by_track.erase(state.track_rid)
    _tracks.erase(track_render_rid)


func _reset_track_instances(state: TrackState) -> void:
    if state.primary_rail_mesh_instance.is_valid():
        RenderingServer.instance_set_base(state.primary_rail_mesh_instance, RID())
    if state.secondary_rail_mesh_instance.is_valid():
        RenderingServer.instance_set_base(state.secondary_rail_mesh_instance, RID())
    if state.primary_blade_mesh_instance.is_valid():
        RenderingServer.instance_set_base(state.primary_blade_mesh_instance, RID())
        RenderingServer.instance_set_transform(state.primary_blade_mesh_instance, Transform3D.IDENTITY)
    if state.secondary_blade_mesh_instance.is_valid():
        RenderingServer.instance_set_base(state.secondary_blade_mesh_instance, RID())
        RenderingServer.instance_set_transform(state.secondary_blade_mesh_instance, Transform3D.IDENTITY)
    if state.trackbed_mesh_instance.is_valid():
        RenderingServer.instance_set_base(state.trackbed_mesh_instance, RID())


func set_track_visible(track_rid: RID, visible: bool) -> void:
    var state: TrackState = _tracks.get(track_rid)
    if not state:
        return

    RenderingServer.instance_set_visible(state.primary_rail_mesh_instance, visible)
    RenderingServer.instance_set_visible(state.secondary_rail_mesh_instance, visible)
    RenderingServer.instance_set_visible(state.primary_blade_mesh_instance, visible)
    RenderingServer.instance_set_visible(state.secondary_blade_mesh_instance, visible)
    RenderingServer.instance_set_visible(state.trackbed_mesh_instance, visible)


func set_track_scenario(track_rid: RID, scenario: RID) -> void:
    var state: TrackState = _tracks.get(track_rid)
    if not state:
        return

    RenderingServer.instance_set_scenario(state.primary_rail_mesh_instance, scenario)
    RenderingServer.instance_set_scenario(state.secondary_rail_mesh_instance, scenario)
    RenderingServer.instance_set_scenario(state.primary_blade_mesh_instance, scenario)
    RenderingServer.instance_set_scenario(state.secondary_blade_mesh_instance, scenario)
    RenderingServer.instance_set_scenario(state.trackbed_mesh_instance, scenario)


func set_switch_blade_offsets(track_render_rid: RID, f_offset1: float, f_offset2: float) -> void:
    var state: TrackState = _tracks.get(track_render_rid)
    if not state:
        return

    var switch_layout: Dictionary = _get_switch_blade_layout(
        state.switch_is_right,
        f_offset1,
        f_offset2,
        TrackServer.switch_max_offset
    )
    var primary_offset: float = switch_layout["primary_blade_offset"]
    var secondary_offset: float = switch_layout["secondary_blade_offset"]
    state.primary_blade_offset = primary_offset
    state.secondary_blade_offset = secondary_offset
    if state.primary_blade_mesh_instance.is_valid():
        RenderingServer.instance_set_transform(
            state.primary_blade_mesh_instance,
            _build_blade_transform(state.primary_blade_pivot, state.primary_blade_angle_factor, primary_offset)
        )
    if state.secondary_blade_mesh_instance.is_valid():
        RenderingServer.instance_set_transform(
            state.secondary_blade_mesh_instance,
            _build_blade_transform(state.secondary_blade_pivot, state.secondary_blade_angle_factor, secondary_offset)
        )


func set_track_render_options(
    track_render_rid: RID,
    tex_length: float,
    tex_height: float,
    tex_width: float,
    tex_slope: float,
    material1: String,
    material2: String,
    material_trackbed: String = "",
    railprofile: String = "",
    rail_visible: bool = true,
    ballast_visible: bool = true,
    ):
    var state: TrackState = _tracks.get(track_render_rid)
    if not state:
        return
    state.tex_length = tex_length
    state.tex_height = tex_height
    state.tex_width = tex_width
    state.tex_slope = tex_slope

    state.material1_name = material1
    state.material2_name = material2
    state.material_trackbed_name = material_trackbed
    state.railprofile = railprofile
    state.rail_visible = rail_visible
    state.ballast_visible = ballast_visible
    #rebuild_track(track_render_rid)


## The track's materials by the names it has, as it is built - a streamed track holds them, and their
## textures, only while it is built
func _resolve_materials(state: TrackState) -> void:
    # An unnamed slot has no material at all - MaterialManager would hand back the
    # missing-texture placeholder instead (Track.cpp:485-491 keeps a null handle here).
    state.material1 = (
        MaterialManager.get_material("", state.material1_name, _track_material_options)
        if state.material1_name else null
    )
    state.material2 = (
        MaterialManager.get_material("", state.material2_name, _track_material_options)
        if state.material2_name else null
    )
    state.material_trackbed = (
        MaterialManager.get_material("", state.material_trackbed_name, _track_material_options)
        if state.material_trackbed_name else null
    )
    state.resolved_trackbed_material_valid = false


func _get_track_data(track_rid: RID) -> TrackData:
    var track: TrackData = TrackData.new()
    if not TrackServer.track_exists(track_rid):
        return track

    var curve1: TrackCurve = TrackServer.track_get_curve(track_rid, TrackServer.TRACK_COMMON)
    if not curve1:
        return track

    track.track_rid = track_rid
    track.curve1 = curve1
    track.endpoints = TrackServer.track_get_endpoints(track_rid)
    if track.endpoints.size() > TrackServer.CURVE2_P1:
        track.curve2 = TrackServer.track_get_curve(track_rid, TrackServer.TRACK_DIVERGING)
    track.width = TrackServer.track_get_width(track_rid)
    track.switch_common_endpoint_index = TrackServer.track_get_common_endpoint_index(track_rid)
    track.switch_f_offset1 = TrackServer.switch_get_f_offset1(track_rid)
    track.switch_f_offset2 = TrackServer.switch_get_f_offset2(track_rid)
    return track


## Registers the track for streaming instead of building it: a scenery has thousands of tracks and
## each one is six RenderingServer instances with no visibility range of their own, drawn from
## anywhere on the map. The meshes are built when the camera comes within the streaming draw
## distance of the chunk holding the track (SceneryStreamingServer) and dropped when it leaves.
func stream_track(track_render_rid: RID) -> void:
    var state: TrackState = _tracks.get(track_render_rid)
    if not state:
        return
    if _stream_owner < 0:
        _stream_owner = SceneryStreamingServer.owner_create("tracks", Callable(), _stream_build, _stream_clear)
    var track: TrackData = _get_track_data(state.track_rid)
    if not track.is_valid():
        return
    state.stream_rid = SceneryStreamingServer.stream_register(
        _stream_owner, track_render_rid, track.curve1.p1, 0.0
    )


func _stream_build(track_render_rid: RID, _preloaded: Variant) -> void:
    var state: TrackState = _tracks.get(track_render_rid)
    if not state:
        return
    state.streamed = true
    rebuild_track(track_render_rid)


func _stream_clear(track_render_rid: RID) -> void:
    var state: TrackState = _tracks.get(track_render_rid)
    if not state:
        return
    state.streamed = false
    _clear_track_meshes(state)
    state.material1 = null
    state.material2 = null
    state.material_trackbed = null
    state.resolved_trackbed_material = null
    state.resolved_trackbed_material_valid = false


func _clear_track_meshes(state: TrackState) -> void:
    _reset_track_instances(state)
    state.primary_rail_mesh = null
    state.secondary_rail_mesh = null
    state.primary_blade_mesh = null
    state.secondary_blade_mesh = null
    state.primary_blade_move_direction = Vector3.ZERO
    state.secondary_blade_move_direction = Vector3.ZERO
    state.primary_blade_pivot = Vector3.ZERO
    state.secondary_blade_pivot = Vector3.ZERO
    state.primary_blade_angle_factor = 0.0
    state.secondary_blade_angle_factor = 0.0
    state.trackbed_mesh = null


func rebuild_track(track_render_rid: RID) -> void:
    var state: TrackState = _tracks.get(track_render_rid)
    if not state:
        return
    if state.stream_rid.is_valid() and not state.streamed:
        return # out of range: built by _stream_build() when the camera comes back
    if not TrackServer:
        return
    var track: TrackData = _get_track_data(state.track_rid)
    if not track.is_valid():
        return

    _clear_track_meshes(state)
    _resolve_materials(state)

    var curve1_data: TrackCurve = track.curve1
    var curve2_data: TrackCurve = track.curve2
    var track_width: float = track.width
    var curve1: Curve3D = _build_curve(curve1_data)

    var length: float = curve1.get_baked_length()
    if not length:
        return

    var rail_profile: RailProfile = _get_rail_profile(state.railprofile)
    var trackbed_material: Material = _resolve_trackbed_material(state, track)
    var is_switch_track: bool = _is_switch_track(track)

    if is_switch_track:
        var curve2: Curve3D = _build_curve(curve2_data)

        var primary_vertices: PackedVector3Array = PackedVector3Array()
        var primary_normals: PackedVector3Array = PackedVector3Array()
        var primary_uvs: PackedVector2Array = PackedVector2Array()
        var primary_indices: PackedInt32Array = PackedInt32Array()
        var secondary_vertices: PackedVector3Array = PackedVector3Array()
        var secondary_normals: PackedVector3Array = PackedVector3Array()
        var secondary_uvs: PackedVector2Array = PackedVector2Array()
        var secondary_indices: PackedInt32Array = PackedInt32Array()
        var primary_blade_vertices: PackedVector3Array = PackedVector3Array()
        var primary_blade_normals: PackedVector3Array = PackedVector3Array()
        var primary_blade_uvs: PackedVector2Array = PackedVector2Array()
        var primary_blade_indices: PackedInt32Array = PackedInt32Array()
        var secondary_blade_vertices: PackedVector3Array = PackedVector3Array()
        var secondary_blade_normals: PackedVector3Array = PackedVector3Array()
        var secondary_blade_uvs: PackedVector2Array = PackedVector2Array()
        var secondary_blade_indices: PackedInt32Array = PackedInt32Array()
        var regular_profile: Array = rail_profile.rail
        var blade_profile: Array = rail_profile.blade
        var rail_height: float = abs(float(regular_profile[0][1]))
        var is_right_switch: bool = TrackServer.switch_is_right(state.track_rid)
        state.switch_is_right = is_right_switch
        # MaSzyna Track.cpp:1367-1453 emits switch rail segments.
        var frog_dist: float = _find_switch_frog_distance(curve1, curve2, track_width)
        var primary_sampled_points: PackedVector3Array = _sample_switch_curve_points(curve1, frog_dist)
        var secondary_sampled_points: PackedVector3Array = _sample_switch_curve_points(curve2, frog_dist)
        var blade_sample_count: int = int(ceil(float(_SWITCH_SEGMENT_COUNT) * _SWITCH_BLADE_RATIO))
        var f_offset1: float = track.switch_f_offset1
        var f_offset2: float = track.switch_f_offset2
        var f_max_offset: float = TrackServer.switch_max_offset

        var switch_layout: Dictionary = _get_switch_blade_layout(
            is_right_switch,
            f_offset1,
            f_offset2,
            f_max_offset
        )
        var primary_blade_mirrored: bool = switch_layout["primary_blade_mirrored"]
        var primary_other_mirrored: bool = switch_layout["primary_other_mirrored"]
        var secondary_blade_mirrored: bool = switch_layout["secondary_blade_mirrored"]
        var secondary_other_mirrored: bool = switch_layout["secondary_other_mirrored"]
        var primary_blade_offset: float = switch_layout["primary_blade_offset"]
        var secondary_blade_offset: float = switch_layout["secondary_blade_offset"]

        _append_switch_fixed_rail_geometry(
            primary_vertices,
            primary_normals,
            primary_uvs,
            primary_indices,
            primary_sampled_points,
            regular_profile,
            track_width,
            curve1_data.roll1,
            curve1_data.roll2,
            state.tex_length,
            rail_height,
            primary_other_mirrored,
            primary_blade_mirrored,
            blade_sample_count
        )
        _append_switch_fixed_rail_geometry(
            secondary_vertices,
            secondary_normals,
            secondary_uvs,
            secondary_indices,
            secondary_sampled_points,
            regular_profile,
            track_width,
            curve2_data.roll1,
            curve2_data.roll2,
            state.tex_length,
            rail_height,
            secondary_other_mirrored,
            secondary_blade_mirrored,
            blade_sample_count
        )
        _append_switch_blade_geometry(
            primary_blade_vertices,
            primary_blade_normals,
            primary_blade_uvs,
            primary_blade_indices,
            primary_sampled_points,
            regular_profile,
            blade_profile,
            track_width,
            curve1_data.roll1,
            curve1_data.roll2,
            state.tex_length,
            rail_height,
            primary_blade_mirrored,
            blade_sample_count
        )
        _append_switch_blade_geometry(
            secondary_blade_vertices,
            secondary_blade_normals,
            secondary_blade_uvs,
            secondary_blade_indices,
            secondary_sampled_points,
            regular_profile,
            blade_profile,
            track_width,
            curve2_data.roll1,
            curve2_data.roll2,
            state.tex_length,
            rail_height,
            secondary_blade_mirrored,
            blade_sample_count
        )
        if state.rail_visible:
            state.primary_rail_mesh = _build_array_mesh(primary_vertices, primary_normals, primary_uvs, primary_indices)
        if state.rail_visible:
            state.secondary_rail_mesh = _build_array_mesh(secondary_vertices, secondary_normals, secondary_uvs, secondary_indices)
        if state.rail_visible:
            state.primary_blade_mesh = _build_array_mesh(primary_blade_vertices, primary_blade_normals, primary_blade_uvs, primary_blade_indices)
            state.primary_blade_move_direction = -_build_planar_parallel(primary_sampled_points, 0)
            state.primary_blade_pivot = _get_switch_blade_pivot(
                primary_sampled_points,
                regular_profile,
                track_width,
                curve1_data.roll1,
                curve1_data.roll2,
                primary_blade_mirrored,
                blade_sample_count,
                rail_height
            )
            state.primary_blade_angle_factor = _get_blade_angle_factor(
                primary_sampled_points,
                blade_profile,
                track_width,
                curve1_data.roll1,
                primary_blade_mirrored,
                rail_height,
                state.primary_blade_move_direction,
                state.primary_blade_pivot
            )
            state.primary_blade_offset = primary_blade_offset
        if state.rail_visible:
            state.secondary_blade_mesh = _build_array_mesh(secondary_blade_vertices, secondary_blade_normals, secondary_blade_uvs, secondary_blade_indices)
            state.secondary_blade_move_direction = -_build_planar_parallel(secondary_sampled_points, 0)
            state.secondary_blade_pivot = _get_switch_blade_pivot(
                secondary_sampled_points,
                regular_profile,
                track_width,
                curve2_data.roll1,
                curve2_data.roll2,
                secondary_blade_mirrored,
                blade_sample_count,
                rail_height
            )
            state.secondary_blade_angle_factor = _get_blade_angle_factor(
                secondary_sampled_points,
                blade_profile,
                track_width,
                curve2_data.roll1,
                secondary_blade_mirrored,
                rail_height,
                state.secondary_blade_move_direction,
                state.secondary_blade_pivot
            )
            state.secondary_blade_offset = secondary_blade_offset

        if state.ballast_visible and trackbed_material:
            # Port of switch trackbed assembly from Track.cpp:3236-3302.
            state.trackbed_mesh = _build_switch_trackbed_mesh(state, track, curve1, curve2, rail_height, is_right_switch)
    else:
        var vertices: PackedVector3Array = PackedVector3Array()
        var normals: PackedVector3Array = PackedVector3Array()
        var uvs: PackedVector2Array = PackedVector2Array()
        var indices: PackedInt32Array = PackedInt32Array()
        var rail_height: float = abs(float(rail_profile.rail[0][1]))
        var right_start: Array = _build_rail_section(rail_profile.rail, track_width, curve1_data.roll1, false)
        var right_end: Array = _build_rail_section(rail_profile.rail, track_width, curve1_data.roll2, false)
        var left_start: Array = _build_rail_section(rail_profile.rail, track_width, curve1_data.roll1, true)
        var left_end: Array = _build_rail_section(rail_profile.rail, track_width, curve1_data.roll2, true)

        _append_loft_geometry(vertices, normals, uvs, indices, curve1, right_start, right_end, state.tex_length, rail_height)
        _append_loft_geometry(vertices, normals, uvs, indices, curve1, left_start, left_end, state.tex_length, rail_height)
        if state.rail_visible:
            state.primary_rail_mesh = _build_array_mesh(vertices, normals, uvs, indices)

        if curve2_data:
            var secondary_curve: Curve3D = _build_curve(curve2_data)
            var secondary_vertices: PackedVector3Array = PackedVector3Array()
            var secondary_normals: PackedVector3Array = PackedVector3Array()
            var secondary_uvs: PackedVector2Array = PackedVector2Array()
            var secondary_indices: PackedInt32Array = PackedInt32Array()
            var second_right_start: Array = _build_rail_section(rail_profile.rail, track_width, curve2_data.roll1, false)
            var second_right_end: Array = _build_rail_section(rail_profile.rail, track_width, curve2_data.roll2, false)
            var second_left_start: Array = _build_rail_section(rail_profile.rail, track_width, curve2_data.roll1, true)
            var second_left_end: Array = _build_rail_section(rail_profile.rail, track_width, curve2_data.roll2, true)
            _append_loft_geometry(secondary_vertices, secondary_normals, secondary_uvs, secondary_indices, secondary_curve, second_right_start, second_right_end, state.tex_length, rail_height)
            _append_loft_geometry(secondary_vertices, secondary_normals, secondary_uvs, secondary_indices, secondary_curve, second_left_start, second_left_end, state.tex_length, rail_height)
            if state.rail_visible:
                state.secondary_rail_mesh = _build_array_mesh(secondary_vertices, secondary_normals, secondary_uvs, secondary_indices)

        if state.ballast_visible and trackbed_material:
            var trackbed_vertices: PackedVector3Array = PackedVector3Array()
            var trackbed_normals: PackedVector3Array = PackedVector3Array()
            var trackbed_uvs: PackedVector2Array = PackedVector2Array()
            var trackbed_indices: PackedInt32Array = PackedInt32Array()
            var end_width: float = track_width
            var end_tex_height: float = state.tex_height + _get_roll_fix_height(curve1_data.roll1)
            var end_tex_width: float = state.tex_width
            var end_tex_slope: float = state.tex_slope
            var next_connection: TrackEndpointRef = _get_unique_endpoint_connection(
                state.track_rid,
                TrackServer.CURVE1_P2
            )
            var next_track_rid: RID = next_connection.track_rid if next_connection else RID()
            var next_track: TrackData = _get_track_data(next_track_rid) if next_track_rid.is_valid() else TrackData.new()
            var next_track_state: TrackState = _get_track_state_by_track_rid(next_track_rid)

            if next_connection and next_track.is_valid() and next_track_state and not _track_has_secondary_curve(next_track):
                end_width = next_track.width
                end_tex_height = next_track_state.tex_height + _get_roll_fix_height(
                    _get_track_roll(next_track, next_connection.endpoint_index)
                )
                end_tex_width = next_track_state.tex_width
                end_tex_slope = next_track_state.tex_slope

            var start_section: Array = _build_trackbed_section(
                track_width,
                state.tex_height + _get_roll_fix_height(curve1_data.roll1),
                state.tex_width,
                state.tex_slope,
                curve1_data.roll1,
                rail_height,
                state.resolved_trackbed_texture_length
            )
            var end_section: Array = _build_trackbed_section(
                end_width,
                end_tex_height,
                end_tex_width,
                end_tex_slope,
                curve1_data.roll2,
                rail_height,
                state.resolved_trackbed_texture_length
            )
            _append_loft_geometry(trackbed_vertices, trackbed_normals, trackbed_uvs, trackbed_indices, curve1, start_section, end_section, state.resolved_trackbed_texture_length, rail_height)

            if curve2_data:
                var second_curve: Curve3D = _build_curve(curve2_data)
                var second_start_section: Array = _build_trackbed_section(
                    track_width,
                    state.tex_height + _get_roll_fix_height(curve2_data.roll1),
                    state.tex_width,
                    state.tex_slope,
                    curve2_data.roll1,
                    rail_height,
                    state.resolved_trackbed_texture_length
                )
                var second_end_section: Array = _build_trackbed_section(
                    track_width,
                    state.tex_height + _get_roll_fix_height(curve2_data.roll2),
                    state.tex_width,
                    state.tex_slope,
                    curve2_data.roll2,
                    rail_height,
                    state.resolved_trackbed_texture_length
                )
                _append_loft_geometry(trackbed_vertices, trackbed_normals, trackbed_uvs, trackbed_indices, second_curve, second_start_section, second_end_section, state.resolved_trackbed_texture_length, rail_height)
            state.trackbed_mesh = _build_array_mesh(trackbed_vertices, trackbed_normals, trackbed_uvs, trackbed_indices)

    if state.primary_rail_mesh:
        RenderingServer.instance_set_base(state.primary_rail_mesh_instance, state.primary_rail_mesh.get_rid())
    if state.secondary_rail_mesh:
        RenderingServer.instance_set_base(state.secondary_rail_mesh_instance, state.secondary_rail_mesh.get_rid())
    if state.primary_blade_mesh:
        RenderingServer.instance_set_base(state.primary_blade_mesh_instance, state.primary_blade_mesh.get_rid())
    if state.secondary_blade_mesh:
        RenderingServer.instance_set_base(state.secondary_blade_mesh_instance, state.secondary_blade_mesh.get_rid())
    if state.trackbed_mesh:
        RenderingServer.instance_set_base(state.trackbed_mesh_instance, state.trackbed_mesh.get_rid())

    # The second path of a switch carries its own rail texture; everything else, and a switch
    # that declares none, uses the track's own one.
    var secondary_material: Material = state.material2 if curve2_data and state.material2 else state.material1

    if state.primary_rail_mesh and state.material1:
        RenderingServer.mesh_surface_set_material(
            state.primary_rail_mesh.get_rid(), 0, state.material1.get_rid(),
        )

    if state.secondary_rail_mesh and secondary_material:
        RenderingServer.mesh_surface_set_material(
            state.secondary_rail_mesh.get_rid(), 0, secondary_material.get_rid(),
        )

    if state.primary_blade_mesh and state.material1:
        RenderingServer.mesh_surface_set_material(
            state.primary_blade_mesh.get_rid(), 0, state.material1.get_rid(),
        )

    if state.secondary_blade_mesh and secondary_material:
        RenderingServer.mesh_surface_set_material(
            state.secondary_blade_mesh.get_rid(), 0, secondary_material.get_rid(),
        )

    if state.trackbed_mesh and trackbed_material:
        for surface_index: int in range(state.trackbed_mesh.get_surface_count()):
            RenderingServer.mesh_surface_set_material(
                state.trackbed_mesh.get_rid(), surface_index, trackbed_material.get_rid()
            )

    set_switch_blade_offsets(track_render_rid, track.switch_f_offset1, track.switch_f_offset2)


func _on_topology_rebuilt() -> void:
    for state: TrackState in _tracks.values():
        state.resolved_trackbed_material_valid = false


## The blades follow the manager's animation step for every switch, scenery RIDs included -
## a scenery has no TrackSwitch3D nodes to forward it.
func _on_switch_offset_changed(track_rid: RID, _offset: float) -> void:
    set_switch_blade_offsets(
        _get_track_render_rid_by_track_rid(track_rid),
        TrackServer.switch_get_f_offset1(track_rid),
        TrackServer.switch_get_f_offset2(track_rid)
    )


func _get_track_render_rid_by_track_rid(track_rid: RID) -> RID:
    if not track_rid.is_valid():
        return RID()
    return _track_render_rids_by_track.get(track_rid, RID())


func _build_curve(curve_data: TrackCurve) -> Curve3D:
    var curve: Curve3D = Curve3D.new()
    curve.closed = false
    curve.bake_interval = _get_mesh_curve_bake_interval(curve_data)
    curve.add_point(
        curve_data.p1 + Vector3(0.0, _get_roll_fix_height(curve_data.roll1), 0.0),
        Vector3.ZERO,
        curve_data.c1
    )
    curve.add_point(
        curve_data.p2 + Vector3(0.0, _get_roll_fix_height(curve_data.roll2), 0.0),
        curve_data.c2,
        Vector3.ZERO
    )
    curve.set_point_tilt(0, curve_data.roll1)
    curve.set_point_tilt(1, curve_data.roll2)
    return curve


func _get_mesh_curve_bake_interval(curve_data: TrackCurve) -> float:
    if not is_zero_approx(curve_data.radius):
        return clampf(
            abs(curve_data.radius) * 0.02,
            _MIN_CURVED_MESH_SEGMENT_LENGTH,
            _MAX_MESH_SEGMENT_LENGTH
        )
    if curve_data.c1.is_zero_approx() and curve_data.c2.is_zero_approx():
        return _MAX_MESH_SEGMENT_LENGTH
    return clampf(
        curve_data.p1.distance_to(curve_data.p2) * 0.1,
        _MIN_CURVED_MESH_SEGMENT_LENGTH,
        _MAX_MESH_SEGMENT_LENGTH
    )


func _get_roll_fix_height(roll_degrees: float) -> float:
    return abs(sin(deg_to_rad(roll_degrees)) * 0.75)


func _resolve_trackbed_material(state: TrackState, _track: TrackData) -> Material:
    if not state.resolved_trackbed_material_valid:
        var owner: TrackState = _copy_adjacent_trackbed_material(state)
        var material_name: String = ""
        state.resolved_trackbed_material = null
        if owner:
            material_name = owner.material_trackbed_name if owner.material_trackbed_name else owner.material2_name
            # the neighbour's own material, which it holds only while it is built itself
            state.resolved_trackbed_material = MaterialManager.get_material("", material_name, _track_material_options)
        # TTrack::texture_length() (Track.cpp:2481-2493): the material's size, else the track's own
        var size_length: float = _get_material_texture_length(material_name) if material_name else -1.0
        state.resolved_trackbed_texture_length = size_length if size_length >= 0.0 else state.tex_length
        state.resolved_trackbed_material_valid = true
    return state.resolved_trackbed_material


## The length a material's texture covers ("size:" of its .mat), -1 if it declares none
func _get_material_texture_length(material_name: String) -> float:
    if not _material_texture_lengths.has(material_name):
        _material_texture_lengths[material_name] = MaterialManager.load_material("", material_name).size.y
    return _material_texture_lengths[material_name]


## Port of TTrack::copy_adjacent_trackbed_material() (Track.cpp:3326). A switch carries no
## trackbed texture of its own - its second slot holds the diverging path's rail texture - and
## the short connectors inside a switch group commonly declare "none", so the material is
## borrowed from a neighbour. [param visited] generalises the original's single-level Exclude
## argument, which cannot stop a longer loop of track from recursing forever.
## Returns the track whose own trackbed material is used - its size sets the mapping too.
func _copy_adjacent_trackbed_material(state: TrackState, visited: Dictionary[RID, bool] = {}) -> TrackState:
    if not state or visited.has(state.track_rid):
        return null
    visited[state.track_rid] = true

    if state.material_trackbed_name:
        return state
    var is_switch: bool = TrackServer.track_is_switch(state.track_rid)
    if not is_switch and state.material2_name:
        return state

    for neighbor_track_rid: RID in _get_trackbed_material_sources(state.track_rid, is_switch):
        var owner: TrackState = _copy_adjacent_trackbed_material(
            _get_track_state_by_track_rid(neighbor_track_rid),
            visited
        )
        if owner:
            return owner
    return null


## Neighbours a track may borrow its trackbed material from: the main path's two for a switch
## (Track.cpp:3359), and for a regular track only while it sits next to a switch
## (Track.cpp:3348) - plain track keeps whatever the scenery gave it.
func _get_trackbed_material_sources(track_rid: RID, is_switch: bool) -> Array[RID]:
    var sources: Array[RID] = []
    if is_switch:
        var neighbors: TrackBranchNeighbors = TrackServer.switch_track_get_neighbors(
            track_rid,
            TrackServer.TRACK_COMMON
        )
        sources.append(neighbors.previous_track_rid)
        sources.append(neighbors.next_track_rid)
        return sources

    var has_adjacent_switch: bool = false
    for endpoint_index: TrackServer.EndpointIndex in [
        TrackServer.CURVE1_P1,
        TrackServer.CURVE1_P2,
    ]:
        for connection: TrackEndpointRef in TrackServer.track_get_endpoint_connections(track_rid, endpoint_index):
            sources.append(connection.track_rid)
            has_adjacent_switch = has_adjacent_switch or TrackServer.track_is_switch(connection.track_rid)
    if not has_adjacent_switch:
        sources.clear()
    return sources


func _get_track_state_by_track_rid(track_rid: RID) -> TrackState:
    var track_render_rid: RID = _get_track_render_rid_by_track_rid(track_rid)
    if not track_render_rid.is_valid():
        return null
    return _tracks.get(track_render_rid)


func _has_trackbed_profile_options(state: TrackState) -> bool:
    return (
        state
        and (
            state.material2_name
            or state.material_trackbed_name
            or not is_zero_approx(state.tex_height)
            or not is_zero_approx(state.tex_width)
            or not is_zero_approx(state.tex_slope)
        )
    )


# Port of switch trackbed profile source selection from Track.cpp:2753-2809.
func _build_switch_trackbed_sections(
    state: TrackState,
    track: TrackData,
    switch_track: int,
    start_roll: float,
    end_roll: float,
    rail_height: float
) -> Array:
    var neighbors: TrackBranchNeighbors = TrackServer.switch_track_get_neighbors(state.track_rid, switch_track)
    var previous_track: TrackData = _get_track_data(neighbors.previous_track_rid) if neighbors.previous_track_rid.is_valid() else TrackData.new()
    var next_track: TrackData = _get_track_data(neighbors.next_track_rid) if neighbors.next_track_rid.is_valid() else TrackData.new()
    var previous_state: TrackState = _get_track_state_by_track_rid(neighbors.previous_track_rid)
    var next_state: TrackState = _get_track_state_by_track_rid(neighbors.next_track_rid)

    var source_state: TrackState = state
    if previous_track.is_valid() and _has_trackbed_profile_options(previous_state) and not _track_has_secondary_curve(previous_track):
        source_state = previous_state
    elif next_track.is_valid() and _has_trackbed_profile_options(next_state) and not _track_has_secondary_curve(next_track):
        source_state = next_state

    var start_tex_height: float = source_state.tex_height + _get_roll_fix_height(start_roll)
    var start_tex_width: float = source_state.tex_width
    var start_tex_slope: float = source_state.tex_slope
    var end_width: float = track.width
    var end_tex_height: float = start_tex_height
    var end_tex_width: float = start_tex_width
    var end_tex_slope: float = start_tex_slope

    if next_track.is_valid() and _has_trackbed_profile_options(next_state) and not _track_has_secondary_curve(next_track):
        end_width = next_track.width
        end_tex_height = next_state.tex_height + _get_roll_fix_height(_get_track_roll(next_track, neighbors.next_endpoint_index))
        end_tex_width = next_state.tex_width
        end_tex_slope = next_state.tex_slope

    return [
        _build_trackbed_section(
            track.width,
            start_tex_height,
            start_tex_width,
            start_tex_slope,
            start_roll,
            rail_height,
            state.resolved_trackbed_texture_length
        ),
        _build_trackbed_section(
            end_width,
            end_tex_height,
            end_tex_width,
            end_tex_slope,
            end_roll,
            rail_height,
            state.resolved_trackbed_texture_length
        ),
    ]


func _track_has_secondary_curve(track: TrackData) -> bool:
    return track.curve2 != null


func _track_get_endpoints(track: TrackData) -> PackedVector3Array:
    return track.endpoints


func _get_track_roll(track: TrackData, endpoint_index: int) -> float:
    if not track.is_valid():
        return 0.0
    var curve1: TrackCurve = track.curve1
    var curve2: TrackCurve = track.curve2
    if endpoint_index == TrackServer.CURVE1_P1:
        return curve1.roll1 if curve1 else 0.0
    if endpoint_index == TrackServer.CURVE1_P2:
        return curve1.roll2 if curve1 else 0.0
    if endpoint_index == TrackServer.CURVE2_P1:
        return curve2.roll1 if curve2 else 0.0
    if endpoint_index == TrackServer.CURVE2_P2:
        return curve2.roll2 if curve2 else 0.0
    return 0.0


func _is_switch_track(track: TrackData) -> bool:
    return (
        track.is_valid()
        and _track_has_secondary_curve(track)
        and not track.switch_common_endpoint_index == TrackServer.POINT_NONE
    )


func _get_unique_endpoint_connection(
    track_rid: RID,
    endpoint_index: TrackServer.EndpointIndex
) -> TrackEndpointRef:
    var connections: Array[TrackEndpointRef] = TrackServer.track_get_endpoint_connections(
        track_rid,
        endpoint_index
    )
    if not connections.size() == 1:
        return null
    return connections[0]


func _get_section_center(section: Array) -> Vector3:
    if not section:
        return Vector3.ZERO
    var center: Vector3 = Vector3.ZERO
    for point: Array in section:
        center += point[0] as Vector3
    return center / float(section.size())


func _get_section_world_center(
    sampled_points: PackedVector3Array,
    sample_index: int,
    section: Array,
    rail_height: float
) -> Vector3:
    if not sampled_points:
        return Vector3.ZERO
    var safe_sample_index: int = clampi(sample_index, 0, sampled_points.size() - 1)
    var frame: Transform3D = _build_curve_frame(sampled_points, safe_sample_index)
    frame.origin.y += rail_height
    return frame * _get_section_center(section)


func _get_switch_blade_pivot(
    sampled_points: PackedVector3Array,
    regular_profile: Array,
    width: float,
    roll_start: float,
    roll_end: float,
    blade_mirrored: bool,
    blade_sample_count: int,
    rail_height: float
) -> Vector3:
    var sections: Dictionary = _get_switch_blade_sections(
        sampled_points,
        regular_profile,
        regular_profile,
        width,
        roll_start,
        roll_end,
        blade_mirrored,
        blade_sample_count
    )
    return _get_section_world_center(sampled_points, blade_sample_count, sections["reg_blade"], rail_height)


func _get_blade_angle_factor(
    sampled_points: PackedVector3Array,
    blade_profile: Array,
    width: float,
    roll_start: float,
    blade_mirrored: bool,
    rail_height: float,
    move_direction: Vector3,
    pivot: Vector3
) -> float:
    if not sampled_points:
        return 0.0
    var blade_start_section: Array = _build_rail_section(blade_profile, width, roll_start, blade_mirrored)
    var blade_start: Vector3 = _get_section_world_center(sampled_points, 0, blade_start_section, rail_height)
    var per_radian_motion: Vector3 = Vector3.UP.cross(blade_start - pivot)
    return move_direction.dot(per_radian_motion)


func _build_blade_transform(pivot: Vector3, angle_factor: float, offset: float) -> Transform3D:
    if is_zero_approx(angle_factor):
        return Transform3D(Basis.IDENTITY, Vector3.ZERO)
    var angle: float = offset / angle_factor
    return Transform3D(Basis(Vector3.UP, angle), pivot) * Transform3D(Basis.IDENTITY, -pivot)


func _get_switch_blade_sections(
    sampled_points: PackedVector3Array,
    regular_profile: Array,
    blade_profile: Array,
    width: float,
    roll_start: float,
    roll_end: float,
    blade_mirrored: bool,
    blade_sample_count: int
) -> Dictionary:
    var transition_sample_count: int = maxi(int(blade_sample_count / 2), 1)
    var total_dist: float = _get_baked_segment_length(sampled_points, 0, -1)
    if total_dist <= 0.0:
        total_dist = 0.001

    var d_trans: float = _get_baked_segment_length(sampled_points, 0, transition_sample_count)
    var d_blade: float = _get_baked_segment_length(sampled_points, 0, blade_sample_count)
    var roll_trans: float = lerpf(roll_start, roll_end, d_trans / total_dist)
    var roll_blade: float = lerpf(roll_start, roll_end, d_blade / total_dist)

    return {
        "transition_sample_count": transition_sample_count,
        "blade_start": _build_rail_section(blade_profile, width, roll_start, blade_mirrored),
        "reg_trans": _build_rail_section(regular_profile, width, roll_trans, blade_mirrored),
        "reg_blade": _build_rail_section(regular_profile, width, roll_blade, blade_mirrored),
        "reg_end": _build_rail_section(regular_profile, width, roll_end, blade_mirrored),
    }


# Fixed rail parts are generated once; moving blades are separate mesh instances.
# MaSzyna Track.cpp:1372-1443 separates blades from fixed rails.
func _append_switch_fixed_rail_geometry(
    vertices: PackedVector3Array,
    normals: PackedVector3Array,
    uvs: PackedVector2Array,
    indices: PackedInt32Array,
    sampled_points: PackedVector3Array,
    regular_profile: Array,
    width: float,
    roll_start: float,
    roll_end: float,
    tex_length: float,
    rail_height: float,
    other_mirrored: bool,
    blade_mirrored: bool,
    blade_sample_count: int
) -> void:
    var other_start: Array = _build_rail_section(regular_profile, width, roll_start, other_mirrored)
    var other_end: Array = _build_rail_section(regular_profile, width, roll_end, other_mirrored)
    _append_loft_geometry_points(vertices, normals, uvs, indices, sampled_points, other_start, other_end, tex_length, rail_height)

    var sections: Dictionary = _get_switch_blade_sections(
        sampled_points,
        regular_profile,
        regular_profile,
        width,
        roll_start,
        roll_end,
        blade_mirrored,
        blade_sample_count
    )
    _append_loft_geometry_range_points(
        vertices,
        normals,
        uvs,
        indices,
        sampled_points,
        sections["reg_blade"],
        sections["reg_end"],
        tex_length,
        rail_height,
        blade_sample_count,
        -1
    )


# The blade mesh is built at zero offset; domain fOffsetX is applied as instance transform.
# MaSzyna Track.cpp:1975-2007 updates blade geometry only.
# MaSzyna Segment.cpp:376-524 applies fOffsetX laterally.
func _append_switch_blade_geometry(
    vertices: PackedVector3Array,
    normals: PackedVector3Array,
    uvs: PackedVector2Array,
    indices: PackedInt32Array,
    sampled_points: PackedVector3Array,
    regular_profile: Array,
    blade_profile: Array,
    width: float,
    roll_start: float,
    roll_end: float,
    tex_length: float,
    rail_height: float,
    blade_mirrored: bool,
    blade_sample_count: int
) -> void:
    var sections: Dictionary = _get_switch_blade_sections(
        sampled_points,
        regular_profile,
        blade_profile,
        width,
        roll_start,
        roll_end,
        blade_mirrored,
        blade_sample_count
    )
    var transition_sample_count: int = sections["transition_sample_count"]
    _append_loft_geometry_range_points(
        vertices,
        normals,
        uvs,
        indices,
        sampled_points,
        sections["blade_start"],
        sections["reg_trans"],
        tex_length,
        rail_height,
        0,
        transition_sample_count,
        0.0,
        0.0
    )
    _append_loft_geometry_range_points(
        vertices,
        normals,
        uvs,
        indices,
        sampled_points,
        sections["reg_trans"],
        sections["reg_blade"],
        tex_length,
        rail_height,
        transition_sample_count,
        blade_sample_count,
        0.0
    )


func _build_rail_section(profile: Array, width: float, roll: float, mirrored: bool) -> Array:
    var section: Array = []
    var half_width: float = 0.5 * abs(width)
    # The loft frames here map the profile's x to the left of the track direction, the original's
    # RenderLoft to the right (Segment.cpp:398 parallel): the profile is mirrored, so the cant
    # (Track.cpp:2652) turns the other way
    var roll_radians: float = -deg_to_rad(roll)
    var sin_roll: float = sin(roll_radians)
    var cos_roll: float = cos(roll_radians)

    for point_data: Array in profile:
        var x: float = float(point_data[0])
        var y: float = float(point_data[1])
        var nx: float = float(point_data[2])
        var ny: float = float(point_data[3])
        var uv_x: float = float(point_data[4])

        var position: Vector3
        var normal: Vector3
        if mirrored:
            position = Vector3(
                (-half_width - x) * cos_roll + y * sin_roll,
                -(-half_width - x) * sin_roll + y * cos_roll,
                0.0
            )
            normal = Vector3(
                -nx * cos_roll + ny * sin_roll,
                nx * sin_roll + ny * cos_roll,
                0.0
            )
            section.push_front([position, normal.normalized(), uv_x])
        else:
            position = Vector3(
                (half_width + x) * cos_roll + y * sin_roll,
                -(half_width + x) * sin_roll + y * cos_roll,
                0.0
            )
            normal = Vector3(
                nx * cos_roll + ny * sin_roll,
                -nx * sin_roll + ny * cos_roll,
                0.0
            )
            section.append([position, normal.normalized(), uv_x])

    return section


func _get_switch_blade_layout(
    is_right_switch: bool,
    f_offset1: float,
    f_offset2: float,
    f_max_offset: float
) -> Dictionary:
    if is_right_switch:
        return {
            "primary_blade_mirrored": true,
            "primary_other_mirrored": false,
            "secondary_blade_mirrored": false,
            "secondary_other_mirrored": true,
            "primary_blade_offset": f_offset2,
            "secondary_blade_offset": -f_max_offset + f_offset1,
        }

    return {
        "primary_blade_mirrored": false,
        "primary_other_mirrored": true,
        "secondary_blade_mirrored": true,
        "secondary_other_mirrored": false,
        "primary_blade_offset": -f_offset2,
        "secondary_blade_offset": f_max_offset - f_offset1,
    }


# Port of trackbed profile construction from Track.cpp:2825-2982.
func _build_trackbed_section(
    width: float,
    tex_height: float,
    tex_width: float,
    tex_slope: float,
    roll: float,
    rail_height: float,
    tex_length: float
) -> Array:
    var half_width: float = 0.5 * abs(width)
    var side: float = abs(tex_width)
    var slope_abs: float = abs(tex_slope)
    var spread: float = half_width + side + slope_abs
    var hypot_value: float = sqrt(slope_abs * slope_abs + tex_height * tex_height)
    if is_zero_approx(hypot_value):
        hypot_value = 1.0

    var normal_x: float = tex_height / hypot_value
    var normal_y: float = tex_slope / hypot_value
    if is_zero_approx(normal_x) and is_zero_approx(normal_y):
        normal_x = 1.0
        normal_y = 0.0

    # mirrored like the rail profile (_build_rail_section)
    var roll_radians: float = -deg_to_rad(roll)
    var sin_roll: float = sin(roll_radians)
    var cos_roll: float = cos(roll_radians)
    # the texture's width is proportional to its length (fTexRatio2 is 1 for rail track,
    # Track.cpp:2930-2932); a 4 m texture keeps the old fixed mapping (Track.cpp:2855-2926)
    var safe_tex_length: float = max(abs(tex_length), 0.001)
    var u_inner: float = 0.5 - (half_width + side) / safe_tex_length
    var u_outer: float = 0.5 - (half_width + side + hypot_value) / safe_tex_length
    var slope_normal: Vector3 = Vector3(normal_x, normal_y, 0.0).normalized()
    if is_equal_approx(tex_length, _TRACKBED_OLD_MAPPING_TEXTURE_LENGTH):
        u_inner = _TRACKBED_OLD_MAPPING_BREAK_U
        u_outer = 0.0
        slope_normal = Vector3(
            cos(deg_to_rad(_TRACKBED_OLD_MAPPING_NORMAL_ANGLE)), sin(deg_to_rad(_TRACKBED_OLD_MAPPING_NORMAL_ANGLE)), 0.0
        )

    return [
        [
            Vector3(
                spread * cos_roll - tex_height * sin_roll,
                -spread * sin_roll - tex_height * cos_roll - rail_height,
                0.0
            ),
            slope_normal,
            u_outer
        ],
        [
            Vector3(
                (half_width + side) * cos_roll,
                -(half_width + side) * sin_roll - rail_height,
                0.0
            ),
            slope_normal,
            u_inner
        ],
        [
            Vector3(0.0, -rail_height + 0.01, 0.0),
            Vector3.UP,
            0.5
        ],
        [
            Vector3(
                -(half_width + side) * cos_roll,
                (half_width + side) * sin_roll - rail_height,
                0.0
            ),
            Vector3(-slope_normal.x, slope_normal.y, 0.0),
            1.0 - u_inner
        ],
        [
            Vector3(
                -spread * cos_roll - tex_height * sin_roll,
                spread * sin_roll - tex_height * cos_roll - rail_height,
                0.0
            ),
            Vector3(-slope_normal.x, slope_normal.y, 0.0),
            1.0 - u_outer
        ],
    ]


func _append_loft_geometry(
    vertices: PackedVector3Array,
    normals: PackedVector3Array,
    uvs: PackedVector2Array,
    indices: PackedInt32Array,
    curve: Curve3D,
    start_section: Array,
    end_section: Array,
    tex_length: float,
    rail_height: float
) -> void:
    _append_loft_geometry_points(
        vertices,
        normals,
        uvs,
        indices,
        curve.get_baked_points(),
        start_section,
        end_section,
        tex_length,
        rail_height
    )


func _append_loft_geometry_points(
    vertices: PackedVector3Array,
    normals: PackedVector3Array,
    uvs: PackedVector2Array,
    indices: PackedInt32Array,
    sampled_points: PackedVector3Array,
    start_section: Array,
    end_section: Array,
    tex_length: float,
    rail_height: float
) -> void:
    _append_loft_geometry_range_points(
        vertices,
        normals,
        uvs,
        indices,
        sampled_points,
        start_section,
        end_section,
        tex_length,
        rail_height,
        0,
        -1
    )


func _append_loft_geometry_range_points(
    vertices: PackedVector3Array,
    normals: PackedVector3Array,
    uvs: PackedVector2Array,
    indices: PackedInt32Array,
    sampled_points: PackedVector3Array,
    start_section: Array,
    end_section: Array,
    tex_length: float,
    rail_height: float,
    start_sample: int,
    end_sample: int,
    lateral_offset_start: float = 0.0,
    lateral_offset_end: float = 0.0
) -> void:
    if start_section.size() < 2 or not start_section.size() == end_section.size():
        return

    if sampled_points.size() < 2:
        return

    var first_sample: int = clampi(start_sample, 0, sampled_points.size() - 1)
    var last_sample: int = sampled_points.size() - 1 if end_sample < 0 else clampi(end_sample, 0, sampled_points.size() - 1)
    if last_sample <= first_sample:
        return

    var safe_tex_length: float = max(abs(tex_length), 0.001)
    var section_size: int = start_section.size()
    var base_vertex: int = vertices.size()
    var distance: float = 0.0
    var total_length: float = max(_get_baked_segment_length(sampled_points, first_sample, last_sample), 0.001)
    var segment_step_count: int = max(last_sample - first_sample, 1)

    for sample_index: int in range(first_sample, last_sample + 1):
        var t: float = distance / total_length
        var offset_t: float = float(sample_index - first_sample) / float(segment_step_count)
        var frame: Transform3D = _build_curve_frame(sampled_points, sample_index)
        frame.origin.y += rail_height

        for point_index: int in range(section_size):
            var start_point: Array = start_section[point_index]
            var end_point: Array = end_section[point_index]

            var local_position: Vector3 = (start_point[0] as Vector3).lerp(end_point[0] as Vector3, t)
            local_position.x += lerpf(lateral_offset_start, lateral_offset_end, offset_t)
            var local_normal: Vector3 = (start_point[1] as Vector3).lerp(end_point[1] as Vector3, t).normalized()
            var uv_x: float = lerpf(float(start_point[2]), float(end_point[2]), t)

            vertices.push_back(frame * local_position)
            normals.push_back((frame.basis * local_normal).normalized())
            uvs.push_back(Vector2(uv_x, distance / safe_tex_length))

        if sample_index < last_sample:
            distance += sampled_points[sample_index].distance_to(sampled_points[sample_index + 1])

    _append_loft_strip_indices(indices, last_sample - first_sample + 1, section_size, base_vertex)


# Port of switch trackbed generation and assembly from Track.cpp:3236-3302.
func _build_switch_trackbed_mesh(
    state: TrackState,
    track: TrackData,
    primary_curve: Curve3D,
    secondary_curve: Curve3D,
    rail_height: float,
    is_right_switch: bool
) -> ArrayMesh:
    var trackbed_vertices: PackedVector3Array = PackedVector3Array()
    var trackbed_normals: PackedVector3Array = PackedVector3Array()
    var trackbed_uvs: PackedVector2Array = PackedVector2Array()
    var trackbed_indices: PackedInt32Array = PackedInt32Array()
    var curve1: TrackCurve = track.curve1
    var curve2: TrackCurve = track.curve2
    var primary_sections: Array = _build_switch_trackbed_sections(state, track, 0, curve1.roll1, curve1.roll2, rail_height)
    var secondary_sections: Array = _build_switch_trackbed_sections(state, track, 1, curve2.roll1, curve2.roll2, rail_height)

    # Port of per-branch loft generation from Track.cpp:3250-3259.
    var primary_chunks: Array = _build_transition_loft_strip_chunks(
        primary_curve,
        primary_sections[0],
        primary_sections[1],
        state.resolved_trackbed_texture_length,
        rail_height,
    )
    var secondary_chunks: Array = _build_transition_loft_strip_chunks(
        secondary_curve,
        secondary_sections[0],
        secondary_sections[1],
        state.resolved_trackbed_texture_length,
        rail_height,
    )

    var segment_count: int = mini(primary_chunks.size(), secondary_chunks.size())

    for segment_index: int in range(segment_count):
        var primary_chunk: Array = _duplicate_strip_chunk(primary_chunks[segment_index])
        var secondary_chunk: Array = _duplicate_strip_chunk(secondary_chunks[segment_index])

        # Port of samplersoffset-based edge lowering from Track.cpp:3265-3299.
        # [2, 3] is the profile's +x edge, to the left of the track (_build_transition_loft_strip_chunks)
        var primary_fallback_edge: Array = [6, 7] if is_right_switch else [2, 3]
        var secondary_fallback_edge: Array = [2, 3] if is_right_switch else [6, 7]
        var primary_lower_edge: Array = _get_nearest_strip_inner_edge_indices(
            primary_chunk,
            secondary_chunk,
            primary_fallback_edge
        )
        var secondary_lower_edge: Array = _get_nearest_strip_inner_edge_indices(
            secondary_chunk,
            primary_chunk,
            secondary_fallback_edge
        )
        _lower_strip_chunk_vertices(primary_chunk, primary_lower_edge)
        _lower_strip_chunk_vertices(secondary_chunk, secondary_lower_edge)

        # Port of the per-branch output order from Track.cpp:3280-3297.
        # Segment.cpp:452-492 emits one 10-vertex strip chunk at a time; only that chunk
        # describes loft faces. Triangulating across chunk boundaries exposes strip
        # connector triangles as visible geometry in Godot.
        var primary_base_vertex: int = trackbed_vertices.size()
        _append_strip_vertices(trackbed_vertices, trackbed_normals, trackbed_uvs, primary_chunk)
        _append_strip_chunk_triangles(trackbed_indices, primary_base_vertex, primary_chunk.size())

        var secondary_base_vertex: int = trackbed_vertices.size()
        _append_strip_vertices(trackbed_vertices, trackbed_normals, trackbed_uvs, secondary_chunk)
        _append_strip_chunk_triangles(trackbed_indices, secondary_base_vertex, secondary_chunk.size())

    return _build_array_mesh(trackbed_vertices, trackbed_normals, trackbed_uvs, trackbed_indices)


# Port of the transition branch of TSegment::RenderLoft from Segment.cpp:376-524.
func _build_transition_loft_strip_chunks(
    curve: Curve3D,
    start_section: Array,
    end_section: Array,
    tex_length: float,
    rail_height: float
) -> Array:
    if start_section.size() < _TRACKBED_SECTION_POINT_COUNT or not start_section.size() == end_section.size():
        return []

    # Port of fixed switch segmentation from Segment.cpp:128-140.
    var sampled_points: PackedVector3Array = _sample_curve_points(curve, _SWITCH_SEGMENT_COUNT)
    if sampled_points.size() < 2:
        return []

    var chunks: Array = []
    var safe_tex_length: float = max(abs(tex_length), 0.001)
    var segment_count: int = sampled_points.size() - 1
    var tv1: float = 1.0

    for segment_index: int in range(segment_count):
        # Port note for Segment.cpp:398-399 and Track.cpp:644-646 / 708-710:
        # original switch segments used by RenderLoft are already lifted by railheight.
        var pos1: Vector3 = sampled_points[segment_index] + Vector3(0.0, rail_height, 0.0)
        var pos2: Vector3 = sampled_points[segment_index + 1] + Vector3(0.0, rail_height, 0.0)
        # to the left, as every loft here maps the profile's x (_build_rail_section)
        var parallel1: Vector3 = -_build_planar_parallel(sampled_points, segment_index)
        var parallel2: Vector3 = -_build_planar_parallel(sampled_points, segment_index + 1)
        var m1: float = float(segment_index) / float(segment_count)
        var m2: float = float(segment_index + 1) / float(segment_count)
        var jmm1: float = 1.0 - m1
        var jmm2: float = 1.0 - m2
        var segment_length: float = pos1.distance_to(pos2)
        tv1 = _clamp_circular(tv1, 1.0)
        var tv2: float = tv1 - segment_length / safe_tex_length
        var chunk: Array = []

        # Port of per-segment strip vertex generation from Segment.cpp:452-492.
        for point_index: int in range(_TRACKBED_SECTION_POINT_COUNT):
            var start_point: Array = start_section[point_index]
            var end_point: Array = end_section[point_index]
            var start_x: float = jmm1 * (start_point[0] as Vector3).x + m1 * (end_point[0] as Vector3).x
            var start_y: float = jmm1 * (start_point[0] as Vector3).y + m1 * (end_point[0] as Vector3).y
            var end_x: float = jmm2 * (start_point[0] as Vector3).x + m2 * (end_point[0] as Vector3).x
            var end_y: float = jmm2 * (start_point[0] as Vector3).y + m2 * (end_point[0] as Vector3).y
            var start_normal_x: float = jmm1 * (start_point[1] as Vector3).x + m1 * (end_point[1] as Vector3).x
            var start_normal_y: float = jmm1 * (start_point[1] as Vector3).y + m1 * (end_point[1] as Vector3).y
            var end_normal_x: float = jmm2 * (start_point[1] as Vector3).x + m2 * (end_point[1] as Vector3).x
            var end_normal_y: float = jmm2 * (start_point[1] as Vector3).y + m2 * (end_point[1] as Vector3).y
            var uv_x_start: float = jmm1 * float(start_point[2]) + m1 * float(end_point[2])
            var uv_x_end: float = jmm2 * float(start_point[2]) + m2 * float(end_point[2])

            chunk.append([
                pos1 + parallel1 * start_x + Vector3(0.0, start_y, 0.0),
                (parallel1 * start_normal_x + Vector3(0.0, start_normal_y, 0.0)).normalized(),
                Vector2(uv_x_start, tv1)
            ])
            chunk.append([
                pos2 + parallel2 * end_x + Vector3(0.0, end_y, 0.0),
                (parallel2 * end_normal_x + Vector3(0.0, end_normal_y, 0.0)).normalized(),
                Vector2(uv_x_end, tv2)
            ])

        chunks.append(chunk)
        tv1 = tv2

    return chunks


func _find_switch_frog_distance(curve1: Curve3D, curve2: Curve3D, target_width: float) -> float:
    var length: float = minf(curve1.get_baked_length(), curve2.get_baked_length())
    var step: float = 0.5
    var dist: float = 0.0
    while dist < length:
        var p1: Vector3 = curve1.sample_baked(dist)
        var p2: Vector3 = curve2.sample_baked(dist)
        if p1.distance_to(p2) >= target_width:
            return dist
        dist += step
    return length


func _sample_curve_points_range(curve: Curve3D, start_dist: float, end_dist: float, segment_count: int) -> PackedVector3Array:
    var points: PackedVector3Array = PackedVector3Array()
    var length: float = end_dist - start_dist
    if length <= 0.0:
        return points
    for i: int in range(segment_count + 1):
        var d: float = start_dist + length * float(i) / float(segment_count)
        points.push_back(curve.sample_baked(d, true))
    return points


func _sample_switch_curve_points(curve: Curve3D, frog_dist: float) -> PackedVector3Array:
    var points: PackedVector3Array = _sample_curve_points_range(
        curve,
        0.0,
        frog_dist,
        _SWITCH_SEGMENT_COUNT
    )
    var total_length: float = curve.get_baked_length()
    if total_length > frog_dist + 0.05:
        var rem_segments: int = maxi(1, int((total_length - frog_dist) / 2.0))
        var rem_points: PackedVector3Array = _sample_curve_points_range(curve, frog_dist, total_length, rem_segments)
        for i: int in range(1, rem_points.size()):
            points.push_back(rem_points[i])
    return points


# Port-side adapter for the fixed switch segmentation used by Segment.cpp:128-140.
func _sample_curve_points(curve: Curve3D, segment_count: int) -> PackedVector3Array:
    var points: PackedVector3Array = PackedVector3Array()
    var baked_length: float = curve.get_baked_length()
    if baked_length <= 0.0 or segment_count <= 0:
        return points
    for sample_index: int in range(segment_count + 1):
        var distance: float = baked_length * float(sample_index) / float(segment_count)
        points.push_back(curve.sample_baked(distance, true))
    return points


# Port of planar cross-track vector construction from Segment.cpp:398-405 and Segment.cpp:444-450.
func _build_planar_parallel(baked_points: PackedVector3Array, sample_index: int) -> Vector3:
    var tangent: Vector3
    if sample_index <= 0:
        tangent = baked_points[1] - baked_points[0]
    elif sample_index >= baked_points.size() - 1:
        tangent = baked_points[baked_points.size() - 1] - baked_points[baked_points.size() - 2]
    else:
        tangent = baked_points[sample_index + 1] - baked_points[sample_index - 1]

    var parallel: Vector3 = Vector3(-tangent.z, 0.0, tangent.x)
    if parallel.length_squared() <= 0.000001:
        parallel = baked_points[baked_points.size() - 1] - baked_points[0]
        parallel = Vector3(-parallel.z, 0.0, parallel.x)
    if parallel.length_squared() <= 0.000001:
        return Vector3.RIGHT
    return parallel.normalized()


func _duplicate_strip_chunk(chunk: Array) -> Array:
    var copy: Array = []
    for point: Array in chunk:
        copy.append([
            point[0],
            point[1],
            point[2],
        ])
    return copy


# Port of vertex lowering index selection used during switch trackbed assembly from Track.cpp:3271-3293.
func _lower_strip_chunk_vertices(chunk: Array, vertex_indices: Array) -> void:
    for vertex_index: int in vertex_indices:
        if vertex_index < 0 or vertex_index >= chunk.size():
            continue
        var point: Array = chunk[vertex_index]
        point[0] = (point[0] as Vector3) + Vector3(0.0, -_SWITCH_TRACKBED_Z_FIGHT_OFFSET, 0.0)
        chunk[vertex_index] = point


func _get_nearest_strip_inner_edge_indices(chunk: Array, other_chunk: Array, fallback_edge: Array) -> Array:
    if chunk.size() < 8 or other_chunk.size() < 6:
        return fallback_edge

    var other_center: Vector3 = ((other_chunk[4][0] as Vector3) + (other_chunk[5][0] as Vector3)) * 0.5
    var left_edge_center: Vector3 = ((chunk[2][0] as Vector3) + (chunk[3][0] as Vector3)) * 0.5
    var right_edge_center: Vector3 = ((chunk[6][0] as Vector3) + (chunk[7][0] as Vector3)) * 0.5
    var left_delta: Vector3 = left_edge_center - other_center
    var right_delta: Vector3 = right_edge_center - other_center
    left_delta.y = 0.0
    right_delta.y = 0.0

    var left_distance: float = left_delta.length_squared()
    var right_distance: float = right_delta.length_squared()
    if is_equal_approx(left_distance, right_distance):
        return fallback_edge
    return [2, 3] if left_distance < right_distance else [6, 7]


# Port of strip vertex copying from Track.cpp:3280-3297.
func _append_strip_vertices(
    vertices: PackedVector3Array,
    normals: PackedVector3Array,
    uvs: PackedVector2Array,
    chunk: Array
) -> void:
    for point: Array in chunk:
        vertices.push_back(point[0] as Vector3)
        normals.push_back(point[1] as Vector3)
        uvs.push_back(point[2] as Vector2)


# Port of visible strip faces emitted per loft chunk by Segment.cpp:452-492. Wound like
# _append_loft_strip_indices(): the chunk maps the profile's x to the left, so the original's
# order faces up here without reversing it.
func _append_strip_chunk_triangles(
    indices: PackedInt32Array,
    base_vertex: int,
    vertex_count: int
) -> void:
    for point_index: int in range(vertex_count - 2):
        var a: int = base_vertex + point_index
        var b: int = base_vertex + point_index + 1
        var c: int = base_vertex + point_index + 2
        if point_index % 2 == 0:
            indices.push_back(a)
            indices.push_back(b)
            indices.push_back(c)
        else:
            indices.push_back(b)
            indices.push_back(a)
            indices.push_back(c)


func _get_baked_segment_length(baked_points: PackedVector3Array, start_sample: int, end_sample: int) -> float:
    var length: float = 0.0
    var last: int = baked_points.size() - 1 if end_sample < 0 else end_sample
    for sample_index: int in range(start_sample, last):
        length += baked_points[sample_index].distance_to(baked_points[sample_index + 1])
    return length


func _build_curve_frame(baked_points: PackedVector3Array, sample_index: int) -> Transform3D:
    var origin: Vector3 = baked_points[sample_index]
    var tangent: Vector3
    if sample_index == 0:
        tangent = baked_points[1] - baked_points[0]
    elif sample_index == baked_points.size() - 1:
        tangent = baked_points[sample_index] - baked_points[sample_index - 1]
    else:
        tangent = baked_points[sample_index + 1] - baked_points[sample_index - 1]

    if tangent.length_squared() <= 0.000001:
        tangent = Vector3.FORWARD
    else:
        tangent = tangent.normalized()

    var reference_up: Vector3 = Vector3.UP
    if abs(tangent.dot(reference_up)) > 0.999:
        reference_up = Vector3.RIGHT

    var right: Vector3 = reference_up.cross(tangent).normalized()
    var up: Vector3 = tangent.cross(right).normalized()
    var basis: Basis = Basis(right, up, tangent).orthonormalized()
    return Transform3D(basis, origin)


func _append_loft_strip_indices(
    indices: PackedInt32Array,
    step_count: int,
    section_size: int,
    base_vertex: int
) -> void:
    for step: int in range(step_count - 1):
        var current_base: int = base_vertex + step * section_size
        var next_base: int = base_vertex + (step + 1) * section_size
        for point_index: int in range(section_size - 1):
            var current: int = current_base + point_index
            var current_next: int = current_base + point_index + 1
            var next_current: int = next_base + point_index
            var next_next: int = next_base + point_index + 1

            indices.push_back(current)
            indices.push_back(next_current)
            indices.push_back(current_next)
            indices.push_back(current_next)
            indices.push_back(next_current)
            indices.push_back(next_next)


# Port of clamp_circular helper semantics from utilities.h:277.
func _clamp_circular(value: float, range_limit: float) -> float:
    if is_zero_approx(range_limit):
        return value
    var result: float = fmod(value, range_limit)
    if result < 0.0:
        result += range_limit
    return result


func _build_array_mesh(
    vertices: PackedVector3Array,
    normals: PackedVector3Array,
    uvs: PackedVector2Array,
    indices: PackedInt32Array
) -> ArrayMesh:
    var arrays: Array = []
    arrays.resize(Mesh.ARRAY_MAX)
    arrays[Mesh.ARRAY_VERTEX] = vertices
    arrays[Mesh.ARRAY_NORMAL] = normals
    arrays[Mesh.ARRAY_TEX_UV] = uvs
    arrays[Mesh.ARRAY_INDEX] = indices

    var mesh: ArrayMesh = ArrayMesh.new()
    mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays)
    # regen_normal_maps() computes tangents for normal-mapped rail/ballast textures (the arrays
    # above never set ARRAY_TANGENT), but costs ~7-9ms PER CALL almost independent of mesh size -
    # for a real scenery with hundreds of track segments (2-3 calls each: rail, trackbed, and
    # switch secondary rail), that alone was ~2.1-2.3s of a ~3s scenery load, confirmed by
    # profiling. Skipped here; the tradeoff is flatter-looking (untangented) normal-map shading
    # on rails/ballast specifically, not missing geometry or broken base-color/roughness texturing.
    return mesh


func _get_rail_profile(profile_name: String) -> RailProfile:
    if not profile_name:
        return DEFAULT_RAIL_PROFILE
    var profile:RailProfile = _rail_profile_cache.get(profile_name)
    if not profile:
        profile = _load_rail_profile(profile_name)
        if profile:
            _rail_profile_cache[profile_name] = profile
        else:
            push_error("Rail profile not found: "+profile_name)
            profile = DEFAULT_RAIL_PROFILE
    return profile


func _load_rail_profile(profile_name: String) -> RailProfile:
    var game_dir: String = UserSettings.get_maszyna_game_dir()

    if profile_name.begins_with("railprofile_"):
        profile_name = profile_name.trim_prefix("railprofile_")

    var profile_dir:String = game_dir.path_join("models").path_join("tory")
    var profile_path:String = profile_dir.path_join(MaszynaDataPath.resolve(
        profile_dir, "railprofile_%s.txt" % profile_name
    ))
    if not FileAccess.file_exists(profile_path):
        return null

    var file: FileAccess = FileAccess.open(profile_path, FileAccess.READ)
    if not file:
        return null

    var rail_profile: Array = []
    var blade_profile: Array = []
    var read_blade_profile: bool = false
    while not file.eof_reached():
        var line: String = file.get_line().strip_edges()
        if not line:
            continue
        if line.begins_with("//"):
            if line.begins_with("// switch blade"):
                read_blade_profile = true
            continue

        var matches: Array[RegExMatch] = _rail_profile_regex.search_all(line)
        if matches.size() < 5:
            continue

        var point:Array[float] = [
            float(matches[0].get_string()),
            float(matches[1].get_string()),
            float(matches[2].get_string()),
            float(matches[3].get_string()),
            float(matches[4].get_string()),
        ]
        if read_blade_profile:
            blade_profile.append(point)
        else:
            rail_profile.append(point)

    return RailProfile.new(rail_profile, blade_profile)
