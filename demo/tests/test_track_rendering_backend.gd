extends MaszynaGutTest

## TrackRenderingServer draws a track by the backend its data names: a core TrackData by the default
## one (TrackRenderingServer.BACKEND_SETTING), a .scn's MaszynaTrackData by MaSzyna's. The core's own
## backend draws a plain track

const BACKEND_SETTING: String = "reloaded/tracks/backend"
## The rail profile MaSzyna's backend draws with (models/tory/railprofile_default.txt) comes from the
## fixtures, not the game directory
const FIXTURES_GAME_DIR: String = "res://tests/fixtures"
## A track as far from the world's origin as Wrzosy's are: floats there resolve only millimetres
const FAR_START: Vector3 = Vector3(40000.0, 0.0, 40000.0)
## How near its own origin a track's vertices lie: the rail profile's width, not the world's [m]
const NEAR_ORIGIN: float = 5.0
## The far track's length [m]
const FAR_LENGTH: float = 20.0

var _track_rid: RID
var _render_rid: RID
var _previous_backend: Variant
var _previous_game_dir: String = ""


func before_each() -> void:
    _previous_game_dir = MaszynaGameDirectory.get_game_dir()
    MaszynaGameDirectory.save_game_dir(FIXTURES_GAME_DIR)
    # a track whose data names no backend is drawn by the core's
    _previous_backend = ProjectSettings.get_setting(BACKEND_SETTING)
    ProjectSettings.set_setting(BACKEND_SETTING, &"standard")
    _track_rid = TrackServer.track_create()
    var curve: TrackCurve = TrackCurve.new()
    curve.p1 = Vector3.ZERO
    curve.p2 = Vector3(20.0, 0.0, 0.0)
    TrackServer.track_update_curves(_track_rid, curve, null)
    TrackServer.track_update(_track_rid, TrackServer.TRACK_NORMAL, "plain_track", TrackData.new().width)
    _render_rid = TrackRenderingServer.track_create(_track_rid)
    TrackRenderingServer.track_set_scenario(_render_rid, get_tree().root.world_3d.scenario)


func after_each() -> void:
    TrackRenderingServer.track_free(_render_rid)
    TrackServer.track_free(_track_rid)
    ProjectSettings.set_setting(BACKEND_SETTING, _previous_backend)
    MaszynaGameDirectory.save_game_dir(_previous_game_dir)


func test_a_core_track_is_drawn_by_the_default_backend_with_a_bed_and_two_rails() -> void:
    TrackRenderingServer.track_set_description(_render_rid, TrackData.new())
    TrackRenderingServer.track_rebuild(_render_rid)

    var standard: StandardTrackRenderingBackend = instance_from_id(
            TrackRenderingServer.backend_get(&"standard")) as StandardTrackRenderingBackend
    var mesh: ArrayMesh = standard.track_get_mesh(_render_rid)
    assert_not_null(mesh, "the default backend draws it")
    assert_eq(mesh.get_surface_count(), 2, "the two rails and the bed; no formation where the track gives none")
    # the bed reaches 2 cm past either end, under the next track's (BED_JOINT_OVERLAP)
    assert_almost_eq(mesh.get_aabb().size.x, 20.0 + 2.0 * 0.02, 0.001, "along the whole curve")


func test_a_scn_track_is_drawn_by_maszyna_backend() -> void:
    var scn_track: MaszynaTrackData = MaszynaTrackData.new()
    scn_track.material1 = "rail_screw_used1"
    scn_track.material2 = "1435mm/tpbps-new2"
    TrackRenderingServer.track_set_description(_render_rid, scn_track)
    TrackRenderingServer.track_rebuild(_render_rid)

    var maszyna: MaszynaTrackRenderingBackend = instance_from_id(
            TrackRenderingServer.backend_get(&"maszyna")) as MaszynaTrackRenderingBackend
    assert_not_null(maszyna.track_get_meshes(_render_rid).get(MaszynaTrackRenderingBackend.SLOT_PRIMARY_RAIL),
            "the track is the backend's its data names")


## A track far from the world's origin is drawn relative to its start, where its instances stand:
## its vertices in the world's coordinates shook by millimetres as the camera moved (#351)
func test_a_scn_track_far_away_keeps_its_vertices_near_its_start() -> void:
    var curve: TrackCurve = TrackCurve.new()
    curve.p1 = FAR_START
    curve.p2 = FAR_START + Vector3(FAR_LENGTH, 0.0, 0.0)
    TrackServer.track_update_curves(_track_rid, curve, null)
    var scn_track: MaszynaTrackData = MaszynaTrackData.new()
    scn_track.material1 = "rail_screw_used1"
    scn_track.material2 = "1435mm/tpbps-new2"
    TrackRenderingServer.track_set_description(_render_rid, scn_track)
    TrackRenderingServer.track_rebuild(_render_rid)

    var maszyna: MaszynaTrackRenderingBackend = instance_from_id(
            TrackRenderingServer.backend_get(&"maszyna")) as MaszynaTrackRenderingBackend
    var meshes: Dictionary = maszyna.track_get_meshes(_render_rid)
    for slot: int in [MaszynaTrackRenderingBackend.SLOT_PRIMARY_RAIL, MaszynaTrackRenderingBackend.SLOT_TRACKBED]:
        assert_lt((meshes[slot] as Mesh).get_aabb().position.length(), NEAR_ORIGIN, "relative to the start")
