#pragma once
#include <godot_cpp/classes/curve3d.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace godot {
    /* The geometry of the tracks as the original draws them (Track.cpp, Segment.cpp): rails lofted
     * along a track's curves from a rail profile, a switch's fixed rails, its blades and their
     * pivots, a trackbed of a section, a road's surface and its shoulders or sidewalks. It computes
     * them of a TrackPiece alone - a track's data by value, read by its owner - and keeps no state, so
     * it is safe on any thread (the streaming's worker builds the cells of MaszynaTrackRenderingBackend
     * with it): it knows no track server, material, RenderingServer or streaming. */
    class MaszynaTrackMeshBuilder {
        public:
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
            /// The arrays of one mesh surface being built
            struct Geometry {
                    PackedVector3Array vertices;
                    PackedVector3Array normals;
                    PackedVector2Array uvs;
                    PackedInt32Array indices;
            };
            /// A curve of a track by its values
            struct CurveShape {
                    Vector3 p1;
                    Vector3 c1;
                    Vector3 c2;
                    Vector3 p2;
                    double roll1 = 0.0;
                    double roll2 = 0.0;
                    /// How far apart the curve is sampled as it is drawn [m]
                    double bake_interval = 0.0;
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
            /// Where a blade turns as the switch moves: about its pivot at the root, by its offset over
            /// this factor
            struct BladePose {
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
            /// How a piece is drawn: rails along one curve or two, a switch, or a road
            enum PieceShape {
                PIECE_RAILS,
                PIECE_SWITCH,
                PIECE_ROAD,
            };
            /// What a track is drawn of: its curves, sizes and textures' lengths by value, and the
            /// sections its owner took of it and of its neighbours
            struct TrackPiece {
                    PieceShape shape = PIECE_RAILS;
                    CurveShape curve1;
                    bool has_curve2 = false;
                    CurveShape curve2;
                    double width = 0.0;
                    /// Length a texture of the rails (a road's surface) and of the bed covers [m]
                    double primary_texture_length = 0.0;
                    double bed_texture_length = 0.0;
                    RailProfile rail_profile;
                    /// How high the rails' heads stand over the curve, their profile's lowest point
                    double rail_height = 0.0;
                    bool switch_is_right = false;
                    /// Which rail of each of a switch's curves is its blade
                    SwitchBladeLayout blade_layout;
                    /// The trackbed at the start and the end of the main curve (a switch's main branch)
                    /// and of the second one; empty for no trackbed
                    Section bed_start;
                    Section bed_end;
                    Section second_bed_start;
                    Section second_bed_end;
                    /// A road's surface at its start and end, and its right and left sides, in the
                    /// loft's frame; empty for a part the road does not have
                    Section surface_start;
                    Section surface_end;
                    Section right_start;
                    Section right_end;
                    Section left_start;
                    Section left_end;
            };
            /// What a piece draws, in the world's coordinates
            struct PieceGeometry {
                    /// The rails of the main curve (a switch's fixed rails), a road's surface
                    Geometry primary_rails;
                    Geometry secondary_rails;
                    Geometry primary_blade;
                    Geometry secondary_blade;
                    /// The trackbed, a road's sides
                    Geometry bed;
                    BladePose primary_blade_pose;
                    BladePose secondary_blade_pose;
            };

        private:
            /// One vertex of a strip of the switch trackbed (Segment.cpp:452-492)
            struct StripPoint {
                    Vector3 position;
                    Vector3 normal;
                    Vector2 uv;
            };
            using StripChunk = Vector<StripPoint>;
            /// The sections a blade is lofted through, from its tip to the frog
            struct BladeSections {
                    int transition_sample_count = 1;
                    Section blade_start;
                    Section reg_trans;
                    Section reg_blade;
                    Section reg_end;
            };

            static Ref<Curve3D> _build_curve(const CurveShape &p_curve);
            static Section
            _build_rail_section(const Vector<ProfilePoint> &p_profile, double p_width, double p_roll, bool p_mirrored);
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
            /// A blade's pivot and how far it turns, the blade at the start of p_sampled_points
            static BladePose _build_blade_pose(
                    const PackedVector3Array &p_sampled_points, const RailProfile &p_profile, double p_width,
                    double p_roll_start, double p_roll_end, bool p_blade_mirrored, int p_blade_sample_count,
                    double p_rail_height);
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
            static void _lower_strip_edge(StripChunk &p_chunk, int p_edge_point);
            static int
            _nearest_strip_inner_edge(const StripChunk &p_chunk, const StripChunk &p_other_chunk, int p_fallback_edge);
            static void _append_strip_chunk(Geometry &p_geometry, const StripChunk &p_chunk);
            /// The switch trackbed of both branches (Track.cpp:3236-3302) of the piece's sections
            static void _append_switch_trackbed_geometry(
                    Geometry &p_geometry, const TrackPiece &p_piece, const Ref<Curve3D> &p_primary_curve,
                    const Ref<Curve3D> &p_secondary_curve);

        public:
            /// What p_piece draws, of nothing but p_piece
            static PieceGeometry build_piece_geometry(const TrackPiece &p_piece);
            /// A surface's arrays (Mesh::ARRAY_MAX of them), its vertices relative to p_origin
            static Array build_surface_arrays(const Geometry &p_geometry, const Vector3 &p_origin);
            /// p_source appended to p_geometry, its indices moved past p_geometry's vertices
            static void append_geometry(Geometry &p_geometry, const Geometry &p_source);
            /// The blade's mesh, relative to p_origin, turned about its pivot in the world by p_offset
            static Transform3D build_blade_transform(const Vector3 &p_origin, const BladePose &p_pose, double p_offset);
            /// How much a roll of p_roll_degrees lifts a curve [m] (Segment.cpp:84)
            static double roll_fix_height(double p_roll_degrees);
            static Section build_trackbed_section(
                    double p_width, double p_tex_height, double p_tex_width, double p_tex_slope, double p_roll,
                    double p_rail_height, double p_tex_length);
            static Section build_road_section(double p_width, double p_roll, double p_texture_length, double p_ratio);
            static void build_road_side_sections(
                    const RoadLook &p_road, const Section &p_surface, double p_texture_length, double p_ratio,
                    Section &p_r_right, Section &p_r_left);
            static Section mirror_road_section(const Section &p_section);
    };
} // namespace godot
