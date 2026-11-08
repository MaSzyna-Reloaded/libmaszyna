#pragma once
#include "maszyna/scenery/MaszynaTrackData.hpp"
#include "maszyna/tracks/MaszynaTrackLooks.hpp"
#include "rendering/TrackRenderingBackend.hpp"
#include "tracks/TrackCurve.hpp"
#include "tracks/TrackEndpointRef.hpp"
#include "tracks/TrackServer.hpp"
#include "utils/RIDHash.hpp"
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/curve3d.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/reg_ex.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/transform3d.hpp>

namespace godot {
    /* The tracks drawn as the original draws them (Track.cpp): rails of a rail profile
     * (<game dir>/models/tory/railprofile_<name>.txt), a switch's blades moving with its offsets, a
     * trackbed of a section, a road of its surface and shoulders, each of its textures. A .scn's
     * track (MaszynaTrackData) is drawn with its own textures; any other with the textures its
     * features map to (its MaszynaTrackLooks, track_set_look()). A streamed road is drawn with the roads around it, in
     * a cell of ROAD_CELL_SIZE streamed by itself. Registered with TrackRenderingServer as NAME. */
    class MaszynaTrackRenderingBackend : public TrackRenderingBackend {
            GDCLASS(MaszynaTrackRenderingBackend, TrackRenderingBackend)

        public:
            /// What a track draws, each in a RenderingServer instance of its own
            enum Slot {
                SLOT_PRIMARY_RAIL,
                SLOT_SECONDARY_RAIL,
                SLOT_PRIMARY_BLADE,
                SLOT_SECONDARY_BLADE,
                SLOT_TRACKBED,
                SLOT_MAX,
            };

            /// Its name with TrackRenderingServer, the one MaszynaTrackData names
            static constexpr const char *NAME = MaszynaTrackData::RENDERING_BACKEND;
            /// Side of a road cell [m]: one cell is built in one go by the streaming, so it holds about as
            /// many roads as one frame's build budget fits (a town's ~100 segments)
            static constexpr double ROAD_CELL_SIZE = 125.0;

        private:
            /// One point of a rail profile: its place across the rail, its normal and its u
            struct ProfilePoint {
                    double x = 0.0;
                    double y = 0.0;
                    double normal_x = 0.0;
                    double normal_y = 0.0;
                    double u = 0.0;
            };
            /// The rail's section and the blade's, the rail's for a profile with no blade of its own
            struct RailProfile {
                    Vector<ProfilePoint> rail;
                    Vector<ProfilePoint> blade;
            };
            /// One point of a section swept along a curve, x to the left of the curve's direction
            struct SectionPoint {
                    Vector3 position;
                    Vector3 normal;
                    double u = 0.0;
            };
            using Section = Vector<SectionPoint>;
            /// One vertex of a strip of the switch trackbed (Segment.cpp:452-492)
            struct StripPoint {
                    Vector3 position;
                    Vector3 normal;
                    Vector2 uv;
            };
            using StripChunk = Vector<StripPoint>;
            /// The arrays of one mesh surface being built
            struct Geometry {
                    PackedVector3Array vertices;
                    PackedVector3Array normals;
                    PackedVector2Array uvs;
                    PackedInt32Array indices;
            };
            /// The track's curves and sizes as TrackServer has them, read when the track is built
            struct TrackGeometry {
                    RID track;
                    Ref<TrackCurve> curve1;
                    Ref<TrackCurve> curve2;
                    double width = 0.0;
                    TrackServer::SwitchCommonPoint switch_common_endpoint_index = TrackServer::POINT_NONE;
                    double switch_f_offset1 = 0.0;
                    double switch_f_offset2 = 0.0;

                    bool is_valid() const;
            };
            /// Which rail of each curve of a switch is its blade, and how far the blades are moved
            struct SwitchBladeLayout {
                    bool primary_blade_mirrored = false;
                    bool primary_other_mirrored = false;
                    bool secondary_blade_mirrored = false;
                    bool secondary_other_mirrored = false;
                    double primary_blade_offset = 0.0;
                    double secondary_blade_offset = 0.0;
            };
            /// The sections a blade is lofted through, from its tip to the frog
            struct BladeSections {
                    int transition_sample_count = 1;
                    Section blade_start;
                    Section reg_trans;
                    Section reg_blade;
                    Section reg_end;
            };
            /// A blade turning about its pivot at the root as the switch moves
            struct Blade {
                    Vector3 move_direction;
                    double offset = 0.0;
                    Vector3 pivot;
                    double angle_factor = 0.0;
            };
            /// The dimensions of a road that shape its cross-section - the ones whose change between two
            /// roads makes the first turn into the second (Track.cpp:347-352)
            struct RoadLook {
                    double width = 0.0;
                    double tex_height = 0.0;
                    double tex_width = 0.0;
                    double tex_slope = 0.0;

                    bool same_as(const RoadLook &p_other) const;
            };
            /// What the backend keeps of a track it draws
            struct Track {
                    RID track;
                    RID scenario;
                    bool visible = true;
                    /// Made when the track first draws something there: a plain track has no blades, a road
                    /// no rails
                    RID instances[SLOT_MAX];
                    Ref<ArrayMesh> meshes[SLOT_MAX];
                    /// Where the meshes stand, the start of the track's curve: their vertices are relative to
                    /// it, as floats keep them, and their instances are placed there in the world's precision
                    /// - vertices in the world's coordinates shook as the camera moved (#351)
                    Vector3 origin;
                    Blade primary_blade;
                    Blade secondary_blade;
                    bool switch_is_right = false;
                    double tex_length = MaszynaTrackData::DEFAULT_TEX_LENGTH;
                    double tex_height = 0.0;
                    double tex_width = 0.0;
                    double tex_slope = 0.0;
                    String railprofile = MaszynaTrackData::DEFAULT_RAILPROFILE;
                    /// What a TrackData's features are drawn with - textures and sections by bed and by
                    /// surface - given by its owner (track_set_look()); a .scn's track has its own
                    Ref<MaszynaTrackLooks> looks;
                    String material1_name;
                    String material2_name;
                    String material_trackbed_name;
                    Ref<Material> material1;
                    Ref<Material> material2;
                    Ref<Material> material_trackbed;
                    /// Trackbed material after the neighbour walk, kept until the topology changes
                    Ref<Material> resolved_trackbed_material;
                    /// Length the trackbed texture covers - its material's size, else the track's own
                    /// tex_length (TTrack::texture_length(), Track.cpp:2481-2493); resolved with the material
                    double resolved_trackbed_texture_length = MaszynaTrackData::DEFAULT_TEX_LENGTH;
                    bool resolved_trackbed_material_valid = false;
                    /// A streamed road is drawn by the road cell holding it, not by instances of its own
                    RID road_cell;
            };
            /// The streamed roads of one square of the map in one scenario, drawn as one mesh of a
            /// surface per material: a town has thousands of road segments, each a draw call or two of a
            /// few triangles when drawn by itself. Measured on an imported Olkusz (2026-10-13, 3904
            /// roads): 42-44 ms a frame drawn by themselves against 27-36 ms without them; 5742 roads in
            /// cells: +375 draw calls, the frame time of the scenery without roads, a cell built in 0.9
            /// ms on average, 5.2 ms at most.
            struct RoadCell {
                    RID scenario;
                    Vector2i coords;
                    /// The cell's centre, where its mesh stands and is streamed from (Track::origin)
                    Vector3 origin;
                    /// Track render RIDs of the roads in the cell
                    Vector<RID> tracks;
                    RID stream_rid;
                    bool streamed = false;
                    RID instance;
                    Ref<ArrayMesh> mesh;
                    bool rebuild_queued = false;
            };
            struct RoadCellKey {
                    RID scenario;
                    Vector2i coords;

                    bool operator==(const RoadCellKey &p_other) const;
            };
            struct RoadCellKeyHasher {
                    static uint32_t hash(const RoadCellKey &p_key);
            };

            RIDMap<Track> tracks;
            /// Road cells by their RID, and by their scenario and coordinates
            RIDMap<RoadCell> road_cells;
            HashMap<RoadCellKey, RID, RoadCellKeyHasher> road_cells_by_key;
            int road_stream_owner = 0;
            bool road_rebuild_pending = false;
            /// The rail profile of a track that names none, and of one the data has not got
            RailProfile default_rail_profile;
            /// The rail profiles read so far, by name - the data's, until it is unloaded
            HashMap<String, RailProfile> rail_profiles;
            /// Texture length ("size:" y) by material name, -1 for none - asked once per material
            HashMap<String, double> material_texture_lengths;
            /// Width/height of the texture by material name, asked once per material (roads)
            HashMap<String, double> material_texture_ratios;
            /// Tracks draw both faces - an open rail profile with its front faces culled casts almost nothing
            Dictionary track_material_options;
            /// A number of a rail profile's line
            Ref<RegEx> rail_profile_number;

            static TrackGeometry _read_track_geometry(const RID &p_track);
            static bool _is_road(const RID &p_track, const TrackGeometry &p_geometry);
            static bool _is_switch_track(const TrackGeometry &p_geometry);
            static double _get_track_roll(const TrackGeometry &p_geometry, int p_endpoint_index);
            static double _get_roll_fix_height(double p_roll_degrees);
            static Ref<Curve3D> _build_curve(const Ref<TrackCurve> &p_curve);
            static SwitchBladeLayout
            _switch_blade_layout(bool p_is_right_switch, double p_f_offset1, double p_f_offset2, double p_f_max_offset);
            static Section
            _build_rail_section(const Vector<ProfilePoint> &p_profile, double p_width, double p_roll, bool p_mirrored);
            static Section _build_trackbed_section(
                    double p_width, double p_tex_height, double p_tex_width, double p_tex_slope, double p_roll,
                    double p_rail_height, double p_tex_length);
            static BladeSections _get_switch_blade_sections(
                    const PackedVector3Array &p_sampled_points, const Vector<ProfilePoint> &p_regular_profile,
                    const Vector<ProfilePoint> &p_blade_profile, double p_width, double p_roll_start, double p_roll_end,
                    bool p_blade_mirrored, int p_blade_sample_count);
            static void _append_loft_geometry_range_points(
                    Geometry &p_geometry, const PackedVector3Array &p_sampled_points, const Section &p_start_section,
                    const Section &p_end_section, double p_tex_length, double p_rail_height, int p_start_sample,
                    int p_end_sample);
            static void _append_loft_strip_indices(
                    PackedInt32Array &p_indices, int p_step_count, int p_section_size, int p_base_vertex);
            static Vector3 _get_section_world_center(
                    const PackedVector3Array &p_sampled_points, int p_sample_index, const Section &p_section,
                    double p_rail_height);
            static Vector3 _get_switch_blade_pivot(
                    const PackedVector3Array &p_sampled_points, const Vector<ProfilePoint> &p_regular_profile,
                    double p_width, double p_roll_start, double p_roll_end, bool p_blade_mirrored,
                    int p_blade_sample_count, double p_rail_height);
            static double _get_blade_angle_factor(
                    const PackedVector3Array &p_sampled_points, const Vector<ProfilePoint> &p_blade_profile,
                    double p_width, double p_roll_start, bool p_blade_mirrored, double p_rail_height,
                    const Vector3 &p_move_direction, const Vector3 &p_pivot);
            static Transform3D _build_blade_transform(
                    const Vector3 &p_origin, const Vector3 &p_pivot, double p_angle_factor, double p_offset);
            static void _append_switch_fixed_rail_geometry(
                    Geometry &p_geometry, const PackedVector3Array &p_sampled_points,
                    const Vector<ProfilePoint> &p_regular_profile, double p_width, double p_roll_start,
                    double p_roll_end, double p_tex_length, double p_rail_height, bool p_other_mirrored,
                    bool p_blade_mirrored, int p_blade_sample_count);
            static void _append_switch_blade_geometry(
                    Geometry &p_geometry, const PackedVector3Array &p_sampled_points,
                    const Vector<ProfilePoint> &p_regular_profile, const Vector<ProfilePoint> &p_blade_profile,
                    double p_width, double p_roll_start, double p_roll_end, double p_tex_length, double p_rail_height,
                    bool p_blade_mirrored, int p_blade_sample_count);
            static Vector<StripChunk> _build_transition_loft_strip_chunks(
                    const Ref<Curve3D> &p_curve, const Section &p_start_section, const Section &p_end_section,
                    double p_tex_length, double p_rail_height);
            static double
            _find_switch_frog_distance(const Ref<Curve3D> &p_curve1, const Ref<Curve3D> &p_curve2, double p_width);
            static PackedVector3Array _sample_curve_points_range(
                    const Ref<Curve3D> &p_curve, double p_start_dist, double p_end_dist, int p_segment_count);
            static PackedVector3Array _sample_switch_curve_points(const Ref<Curve3D> &p_curve, double p_frog_dist);
            static Vector3 _build_planar_parallel(const PackedVector3Array &p_points, int p_sample_index);
            static double
            _get_baked_segment_length(const PackedVector3Array &p_points, int p_start_sample, int p_end_sample);
            static Transform3D _build_curve_frame(const PackedVector3Array &p_points, int p_sample_index);
            static PackedVector3Array
            _vertices_relative_to(const PackedVector3Array &p_vertices, const Vector3 &p_origin);
            static Ref<ArrayMesh> _build_array_mesh(const Geometry &p_geometry, const Vector3 &p_origin);
            static Section _build_road_section(double p_width, double p_roll, double p_texture_length, double p_ratio);
            static void _build_road_side_sections(
                    const RoadLook &p_road, const Section &p_surface, double p_texture_length, double p_ratio,
                    Section &p_r_right, Section &p_r_left);
            static Section _mirror_road_section(const Section &p_section);
            static bool _has_trackbed_profile_options(const Track *p_track);
            static void _lower_strip_edge(StripChunk &p_chunk, int p_edge_point);
            static int
            _nearest_strip_inner_edge(const StripChunk &p_chunk, const StripChunk &p_other_chunk, int p_fallback_edge);
            static void _append_strip_chunk(Geometry &p_geometry, const StripChunk &p_chunk);

            /// The track TrackServer's p_track is drawn as by this backend; null for none
            Track *_find_track(const RID &p_track);
            /// The rail profile of the name, read from the game directory the first time it is drawn
            const RailProfile &_load_rail_profile(const String &p_name);
            /// The length a material's texture covers ("size:" of its .mat), -1 if it declares none -
            /// asked of ReloadedMaterialServer the first time
            double _load_texture_length(const String &p_material_name);
            /// The width/height ratio of a material's texture - a road's surface is as wide as its
            /// texture's length times it (fTexRatio, Track.cpp:610-621); 1 when the texture is not known
            double _load_texture_ratio(const String &p_material_name);
            /// TTrack::texture_length() (Track.cpp:2484-2494): the material's size, else the track's own
            double _load_road_texture_length(const Track &p_track, const String &p_material_name);
            void _resolve_materials(Track &p_track) const;
            Ref<Material> _resolve_trackbed_material(Track &p_track);
            const Track *_copy_adjacent_trackbed_material(const Track *p_track, RIDSet &p_visited);
            Vector<RID> _get_trackbed_material_sources(const RID &p_track, bool p_is_switch) const;
            void _build_switch_trackbed_sections(
                    const Track &p_track, const TrackGeometry &p_geometry, TrackServer::SwitchTrack p_switch_track,
                    double p_start_roll, double p_end_roll, double p_rail_height, Section &p_r_start, Section &p_r_end);
            Ref<ArrayMesh> _build_switch_trackbed_mesh(
                    const Track &p_track, const TrackGeometry &p_geometry, const Ref<Curve3D> &p_primary_curve,
                    const Ref<Curve3D> &p_secondary_curve, double p_rail_height, bool p_is_right_switch);
            /// A road drawn by itself (a hand-placed one, not streamed): its surface by the rail's
            /// instance, its sides by the trackbed's
            void _build_road(Track &p_track, const TrackGeometry &p_geometry, const Ref<Curve3D> &p_curve);
            void _append_road_geometry(
                    const Track &p_track, const TrackGeometry &p_geometry, const Ref<Curve3D> &p_curve,
                    Geometry &p_surface, Geometry &p_sides);
            /// Draws the slot's mesh in the slot's instance, made as the slot first draws something
            static void _slot_draw(Track &p_track, Slot p_slot);
            static void _reset_track_instances(Track &p_track);
            static void _clear_track_meshes(Track &p_track);
            void _road_cell_add(const RID &p_track_render, Track &p_track);
            void _road_cell_remove(const RID &p_track_render, Track &p_track);
            void _queue_road_cell_rebuild(RoadCell &p_cell);
            void _rebuild_queued_road_cells();
            void _road_cell_build(const RID &p_cell, const Variant &p_preloaded);
            void _road_cell_clear(const RID &p_cell);
            void _build_road_cell(RoadCell &p_cell);
            void _on_data_unload_requested();
            void _on_data_reload_requested();

        protected:
            static void _bind_methods();

        public:
            MaszynaTrackRenderingBackend();
            ~MaszynaTrackRenderingBackend() override;

            void track_create(const RID &p_track_render, const RID &p_track) override;
            void track_free(const RID &p_track_render) override;
            void track_set_description(const RID &p_track_render, const Ref<TrackData> &p_description) override;
            void track_set_visible(const RID &p_track_render, bool p_visible) override;
            void track_set_scenario(const RID &p_track_render, const RID &p_scenario) override;
            /// A MaszynaTrackLooks: what a track that is not a .scn's is drawn with
            void track_set_look(const RID &p_track_render, const Ref<Resource> &p_look) override;
            void track_set_switch_blade_offsets(const RID &p_track_render, double p_offset1, double p_offset2) override;
            /// A road is streamed in the cell holding it, a track by TrackRenderingServer
            bool track_stream(const RID &p_track_render) override;
            void track_build(const RID &p_track_render) override;
            /// The track's meshes and its materials go as it leaves the camera's range
            void track_clear(const RID &p_track_render) override;
            /// The resolved trackbed materials are taken again, and a road turns into the next one it is
            /// joined to now
            void topology_rebuilt() override;

            /// The meshes the track draws now, by their Slot
            Dictionary track_get_meshes(const RID &p_track_render) const;
            /// The instances the track has made so far, by their Slot
            Dictionary track_get_instances(const RID &p_track_render) const;
            /// What the track is drawn with: its materials' names (material1, material2,
            /// material_trackbed), its texture's tex_length, tex_height, tex_width and tex_slope and its
            /// railprofile, by MaszynaTrackData's names
            Dictionary track_get_look(const RID &p_track_render) const;
            /// The road cell drawing the streamed road; none for any other track
            RID track_get_road_cell(const RID &p_track_render) const;
            bool road_cell_exists(const RID &p_cell) const;
            /// The cell's instance; none before it is first built
            RID road_cell_get_instance(const RID &p_cell) const;
            /// The cell's roads as one mesh while it is built; null otherwise
            Ref<ArrayMesh> road_cell_get_mesh(const RID &p_cell) const;

            /// How far apart a track of p_curve is sampled as it is drawn [m]
            static double mesh_curve_bake_interval(const Ref<TrackCurve> &p_curve);
            /// Which rail of each curve of a switch is its blade, and how far each blade is moved, as a
            /// Dictionary: primary_blade_mirrored, primary_other_mirrored, secondary_blade_mirrored,
            /// secondary_other_mirrored, primary_blade_offset, secondary_blade_offset
            static Dictionary
            switch_blade_layout(bool p_is_right_switch, double p_f_offset1, double p_f_offset2, double p_f_max_offset);
            /// The one track joined to the endpoint of p_track; null when none or several are
            static Ref<TrackEndpointRef> unique_endpoint_connection(const RID &p_track, int p_endpoint_index);
    };
} // namespace godot

VARIANT_ENUM_CAST(MaszynaTrackRenderingBackend::Slot)
