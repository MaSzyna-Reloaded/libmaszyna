#include "maszyna/tracks/MaszynaTrackMeshBuilder.hpp"

#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/core/math.hpp>

namespace godot {
    namespace {
        constexpr Vector3 UP(0.0, 1.0, 0.0);
        constexpr Vector3 RIGHT(1.0, 0.0, 0.0);
        constexpr Vector3 LEFT(-1.0, 0.0, 0.0);
        constexpr Vector3 FORWARD(0.0, 0.0, -1.0);

        /// Original engine: Segment.cpp:128-140 - a switch's curves are drawn in this many segments
        constexpr int SWITCH_SEGMENT_COUNT = 6;
        /// Original engine: Track.cpp:1371 bladelength - the blade's share of the way to the frog
        constexpr double SWITCH_BLADE_RATIO = 0.65;
        /// Original engine: Segment.cpp:84 - a roll lifts the curve by sin(roll) times this [m] (0.75 m
        /// for 1435 mm)
        constexpr double ROLL_FIX_FACTOR = 0.75;
        /// The switch trackbed's edges under the other curve go this much lower, so the two do not fight
        /// where they overlap [m]
        constexpr double SWITCH_TRACKBED_Z_FIGHT_OFFSET = 0.025;
        /// Original engine: Track.cpp:2860-2880 - a trackbed section: its outer edges, its slopes' upper
        /// edges and its crown
        constexpr int TRACKBED_SECTION_POINT_COUNT = 5;
        /// The trackbed section's points of the slopes' upper edges and of the crown (_build_trackbed_section())
        constexpr int TRACKBED_LEFT_EDGE_POINT = 1;
        constexpr int TRACKBED_CENTRE_POINT = 2;
        constexpr int TRACKBED_RIGHT_EDGE_POINT = 3;
        /// Original engine: Segment.cpp:452-492 - a strip chunk holds each section point twice, at the
        /// segment's start and at its end
        constexpr int STRIP_VERTICES_PER_POINT = 2;
        /// Original engine: Track.cpp:2855 - a trackbed texture 4 m long keeps the old fixed mapping
        constexpr double TRACKBED_OLD_MAPPING_TEXTURE_LENGTH = 4.0;
        /// Original engine: Track.cpp:2857-2858 - the old mapping's slope normals, 75 degrees up
        constexpr double TRACKBED_OLD_MAPPING_NORMAL_ANGLE = 75.0;
        /// Original engine: Track.cpp:2869, 2877 - the old mapping's u at the slope's upper edges
        constexpr double TRACKBED_OLD_MAPPING_BREAK_U = 0.33;
        /// Original engine: Track.cpp:2872 - the trackbed's crown lies this much over its edges [m]
        constexpr double TRACKBED_CROWN_LIFT = 0.01;
        /// The least length a texture or a loft is divided by [m]
        constexpr double MIN_LENGTH = 0.001;
        /// A tangent or a parallel shorter than this, squared, gives no direction
        constexpr double MIN_DIRECTION_LENGTH_SQUARED = 0.000001;
        /// A tangent this close to the vertical takes another reference for its frame
        constexpr double VERTICAL_TANGENT_DOT = 0.999;
        /// Step along both curves of a switch while looking for the frog, where they part [m]
        constexpr double FROG_SEARCH_STEP = 0.5;
        /// A switch's curve going on past its frog by more than this has its tail drawn too [m], in
        /// segments of about SWITCH_TAIL_SEGMENT_LENGTH [m]
        constexpr double SWITCH_TAIL_MIN_LENGTH = 0.05;
        constexpr double SWITCH_TAIL_SEGMENT_LENGTH = 2.0;
        /// Original engine: utilities.h:277 clamp_circular() - the switch trackbed's v wraps within this
        /// range, from its top
        constexpr double TEXTURE_V_RANGE = 1.0;
        /// Original engine: Track.cpp:3022 - a road texture of no known width is cut in its middle
        constexpr double ROAD_DEFAULT_MAP = 0.5;
        /// Original engine: Track.cpp:3160-3163 - a curb's face is sloped 1:3.75 (150 mm high, it leans
        /// 40 mm)
        constexpr double CURB_SLOPE = 3.75;
        /// Original engine: Track.cpp:3162 - the curbs are mapped from 31/64 to 32/64 and from 32/64 to
        /// 33/64 of the texture
        constexpr double CURB_LEFT_U = 0.484375;
        constexpr double CURB_RIGHT_U = 0.515625;
        /// Original engine: Track.cpp:3192 - the lean of a curb's face over this is its share of u
        constexpr double CURB_LEAN_PER_U = 2.56;
        /// The loft goes on to the curve's end, its last sampled point
        constexpr int CURVE_END = -1;
    } // namespace

    bool MaszynaTrackMeshBuilder::RoadLook::same_as(const RoadLook &p_other) const {
        return width == p_other.width && tex_height == p_other.tex_height && tex_width == p_other.tex_width &&
               tex_slope == p_other.tex_slope;
    }

    double MaszynaTrackMeshBuilder::roll_fix_height(const double p_roll_degrees) {
        return Math::abs(Math::sin(Math::deg_to_rad(p_roll_degrees)) * ROLL_FIX_FACTOR);
    }

    Ref<Curve3D> MaszynaTrackMeshBuilder::_build_curve(const CurveShape &p_curve) {
        Ref<Curve3D> curve;
        curve.instantiate();
        curve->set_closed(false);
        curve->set_bake_interval(static_cast<real_t>(p_curve.bake_interval));
        curve->add_point(
                p_curve.p1 + Vector3(0.0, static_cast<real_t>(roll_fix_height(p_curve.roll1)), 0.0), Vector3(),
                p_curve.c1);
        curve->add_point(
                p_curve.p2 + Vector3(0.0, static_cast<real_t>(roll_fix_height(p_curve.roll2)), 0.0), p_curve.c2,
                Vector3());
        curve->set_point_tilt(0, static_cast<real_t>(p_curve.roll1));
        curve->set_point_tilt(1, static_cast<real_t>(p_curve.roll2));
        return curve;
    }

    MaszynaTrackMeshBuilder::Section MaszynaTrackMeshBuilder::_build_rail_section(
            const Vector<ProfilePoint> &p_profile, const double p_width, const double p_roll, const bool p_mirrored) {
        Section section;
        const double half_width = 0.5 * Math::abs(p_width);
        // The loft frames here map the profile's x to the left of the track direction, the original's
        // RenderLoft to the right (Segment.cpp:398 parallel): the profile is mirrored, so the cant
        // (Track.cpp:2652) turns the other way
        const double roll_radians = -Math::deg_to_rad(p_roll);
        const double sin_roll = Math::sin(roll_radians);
        const double cos_roll = Math::cos(roll_radians);

        for (const ProfilePoint &point: p_profile) {
            SectionPoint section_point;
            section_point.u = point.u;
            if (p_mirrored) {
                section_point.position =
                        Vector3(static_cast<real_t>(((-half_width - point.x) * cos_roll) + (point.y * sin_roll)),
                                static_cast<real_t>((-(-half_width - point.x) * sin_roll) + (point.y * cos_roll)), 0.0);
                section_point.normal =
                        Vector3(static_cast<real_t>((-point.normal_x * cos_roll) + (point.normal_y * sin_roll)),
                                static_cast<real_t>((point.normal_x * sin_roll) + (point.normal_y * cos_roll)), 0.0)
                                .normalized();
                section.insert(0, section_point);
            } else {
                section_point.position =
                        Vector3(static_cast<real_t>(((half_width + point.x) * cos_roll) + (point.y * sin_roll)),
                                static_cast<real_t>((-(half_width + point.x) * sin_roll) + (point.y * cos_roll)), 0.0);
                section_point.normal =
                        Vector3(static_cast<real_t>((point.normal_x * cos_roll) + (point.normal_y * sin_roll)),
                                static_cast<real_t>((-point.normal_x * sin_roll) + (point.normal_y * cos_roll)), 0.0)
                                .normalized();
                section.push_back(section_point);
            }
        }
        return section;
    }

    // Port of trackbed profile construction from Track.cpp:2825-2982.
    MaszynaTrackMeshBuilder::Section MaszynaTrackMeshBuilder::build_trackbed_section(
            const double p_width, const double p_tex_height, const double p_tex_width, const double p_tex_slope,
            const double p_roll, const double p_rail_height, const double p_tex_length) {
        const double half_width = 0.5 * Math::abs(p_width);
        const double side = Math::abs(p_tex_width);
        const double slope_abs = Math::abs(p_tex_slope);
        const double spread = half_width + side + slope_abs;
        double hypot_value = Math::sqrt((slope_abs * slope_abs) + (p_tex_height * p_tex_height));
        if (Math::is_zero_approx(hypot_value)) {
            hypot_value = 1.0;
        }

        double normal_x = p_tex_height / hypot_value;
        double normal_y = p_tex_slope / hypot_value;
        if (Math::is_zero_approx(normal_x) && Math::is_zero_approx(normal_y)) {
            normal_x = 1.0;
            normal_y = 0.0;
        }

        // mirrored like the rail profile (_build_rail_section())
        const double roll_radians = -Math::deg_to_rad(p_roll);
        const double sin_roll = Math::sin(roll_radians);
        const double cos_roll = Math::cos(roll_radians);
        // the texture's width is proportional to its length (fTexRatio2 is 1 for rail track,
        // Track.cpp:2930-2932); a 4 m texture keeps the old fixed mapping (Track.cpp:2855-2926)
        const double safe_tex_length = MAX(Math::abs(p_tex_length), MIN_LENGTH);
        double u_inner = 0.5 - ((half_width + side) / safe_tex_length);
        double u_outer = 0.5 - ((half_width + side + hypot_value) / safe_tex_length);
        Vector3 slope_normal = Vector3(static_cast<real_t>(normal_x), static_cast<real_t>(normal_y), 0.0).normalized();
        if (Math::is_equal_approx(p_tex_length, TRACKBED_OLD_MAPPING_TEXTURE_LENGTH)) {
            u_inner = TRACKBED_OLD_MAPPING_BREAK_U;
            u_outer = 0.0;
            slope_normal =
                    Vector3(static_cast<real_t>(Math::cos(Math::deg_to_rad(TRACKBED_OLD_MAPPING_NORMAL_ANGLE))),
                            static_cast<real_t>(Math::sin(Math::deg_to_rad(TRACKBED_OLD_MAPPING_NORMAL_ANGLE))), 0.0);
        }
        const Vector3 mirrored_normal(-slope_normal.x, slope_normal.y, 0.0);

        Section section;
        section.push_back(
                {Vector3(static_cast<real_t>((spread * cos_roll) - (p_tex_height * sin_roll)),
                         static_cast<real_t>((-spread * sin_roll) - (p_tex_height * cos_roll) - p_rail_height), 0.0),
                 slope_normal, u_outer});
        section.push_back(
                {Vector3(static_cast<real_t>((half_width + side) * cos_roll),
                         static_cast<real_t>((-(half_width + side) * sin_roll) - p_rail_height), 0.0),
                 slope_normal, u_inner});
        section.push_back({Vector3(0.0, static_cast<real_t>(-p_rail_height + TRACKBED_CROWN_LIFT), 0.0), UP, 0.5});
        section.push_back(
                {Vector3(static_cast<real_t>(-(half_width + side) * cos_roll),
                         static_cast<real_t>(((half_width + side) * sin_roll) - p_rail_height), 0.0),
                 mirrored_normal, 1.0 - u_inner});
        section.push_back(
                {Vector3(static_cast<real_t>((-spread * cos_roll) - (p_tex_height * sin_roll)),
                         static_cast<real_t>((spread * sin_roll) - (p_tex_height * cos_roll) - p_rail_height), 0.0),
                 mirrored_normal, 1.0 - u_outer});
        return section;
    }

    MaszynaTrackMeshBuilder::BladeSections MaszynaTrackMeshBuilder::_get_switch_blade_sections(
            const PackedVector3Array &p_sampled_points, const Vector<ProfilePoint> &p_regular_profile,
            const Vector<ProfilePoint> &p_blade_profile, const double p_width, const double p_roll_start,
            const double p_roll_end, const bool p_blade_mirrored, const int p_blade_sample_count) {
        BladeSections sections;
        sections.transition_sample_count = MAX(p_blade_sample_count / 2, 1);
        double total_dist = _get_baked_segment_length(p_sampled_points, 0, CURVE_END);
        if (total_dist <= 0.0) {
            total_dist = MIN_LENGTH;
        }

        const double d_trans = _get_baked_segment_length(p_sampled_points, 0, sections.transition_sample_count);
        const double d_blade = _get_baked_segment_length(p_sampled_points, 0, p_blade_sample_count);
        const double roll_trans = Math::lerp(p_roll_start, p_roll_end, d_trans / total_dist);
        const double roll_blade = Math::lerp(p_roll_start, p_roll_end, d_blade / total_dist);

        sections.blade_start = _build_rail_section(p_blade_profile, p_width, p_roll_start, p_blade_mirrored);
        sections.reg_trans = _build_rail_section(p_regular_profile, p_width, roll_trans, p_blade_mirrored);
        sections.reg_blade = _build_rail_section(p_regular_profile, p_width, roll_blade, p_blade_mirrored);
        sections.reg_end = _build_rail_section(p_regular_profile, p_width, p_roll_end, p_blade_mirrored);
        return sections;
    }

    void MaszynaTrackMeshBuilder::_append_loft_geometry_range_points(
            Geometry &p_geometry, const PackedVector3Array &p_sampled_points, const Section &p_start_section,
            const Section &p_end_section, const double p_tex_length, const double p_rail_height,
            const int p_start_sample, const int p_end_sample) {
        if (p_start_section.size() < 2 || p_start_section.size() != p_end_section.size()) {
            return;
        }
        if (p_sampled_points.size() < 2) {
            return;
        }

        const int last_index = static_cast<int>(p_sampled_points.size()) - 1;
        const int first_sample = CLAMP(p_start_sample, 0, last_index);
        const int last_sample = p_end_sample < 0 ? last_index : CLAMP(p_end_sample, 0, last_index);
        if (last_sample <= first_sample) {
            return;
        }

        const double safe_tex_length = MAX(Math::abs(p_tex_length), MIN_LENGTH);
        const int section_size = static_cast<int>(p_start_section.size());
        const int base_vertex = static_cast<int>(p_geometry.vertices.size());
        double distance = 0.0;
        const double total_length =
                MAX(_get_baked_segment_length(p_sampled_points, first_sample, last_sample), MIN_LENGTH);

        for (int sample_index = first_sample; sample_index <= last_sample; sample_index++) {
            const double t = distance / total_length;
            Transform3D frame = _build_curve_frame(p_sampled_points, sample_index);
            frame.origin.y += static_cast<real_t>(p_rail_height);

            for (int point_index = 0; point_index < section_size; point_index++) {
                const SectionPoint &start_point = p_start_section[point_index];
                const SectionPoint &end_point = p_end_section[point_index];

                const Vector3 local_position = start_point.position.lerp(end_point.position, static_cast<real_t>(t));
                const Vector3 local_normal =
                        start_point.normal.lerp(end_point.normal, static_cast<real_t>(t)).normalized();
                const double uv_x = Math::lerp(start_point.u, end_point.u, t);

                p_geometry.vertices.push_back(frame.xform(local_position));
                p_geometry.normals.push_back(frame.basis.xform(local_normal).normalized());
                p_geometry.uvs.push_back(
                        Vector2(static_cast<real_t>(uv_x), static_cast<real_t>(distance / safe_tex_length)));
            }

            if (sample_index < last_sample) {
                distance += p_sampled_points[sample_index].distance_to(p_sampled_points[sample_index + 1]);
            }
        }

        _append_loft_strip_indices(p_geometry.indices, last_sample - first_sample + 1, section_size, base_vertex);
    }

    void MaszynaTrackMeshBuilder::_append_loft_strip_indices(
            PackedInt32Array &p_indices, const int p_step_count, const int p_section_size, const int p_base_vertex) {
        for (int step = 0; step < p_step_count - 1; step++) {
            const int current_base = p_base_vertex + (step * p_section_size);
            const int next_base = p_base_vertex + ((step + 1) * p_section_size);
            for (int point_index = 0; point_index < p_section_size - 1; point_index++) {
                const int current = current_base + point_index;
                const int current_next = current_base + point_index + 1;
                const int next_current = next_base + point_index;
                const int next_next = next_base + point_index + 1;

                p_indices.push_back(current);
                p_indices.push_back(next_current);
                p_indices.push_back(current_next);
                p_indices.push_back(current_next);
                p_indices.push_back(next_current);
                p_indices.push_back(next_next);
            }
        }
    }

    Vector3 MaszynaTrackMeshBuilder::_get_section_world_center(
            const PackedVector3Array &p_sampled_points, const int p_sample_index, const Section &p_section,
            const double p_rail_height) {
        if (p_sampled_points.is_empty()) {
            return {};
        }
        const int safe_sample_index = CLAMP(p_sample_index, 0, static_cast<int>(p_sampled_points.size()) - 1);
        Transform3D frame = _build_curve_frame(p_sampled_points, safe_sample_index);
        frame.origin.y += static_cast<real_t>(p_rail_height);
        Vector3 center;
        if (!p_section.is_empty()) {
            for (const SectionPoint &point: p_section) {
                center += point.position;
            }
            center /= static_cast<real_t>(p_section.size());
        }
        return frame.xform(center);
    }

    Vector3 MaszynaTrackMeshBuilder::_get_switch_blade_pivot(
            const PackedVector3Array &p_sampled_points, const Vector<ProfilePoint> &p_regular_profile,
            const double p_width, const double p_roll_start, const double p_roll_end, const bool p_blade_mirrored,
            const int p_blade_sample_count, const double p_rail_height) {
        const BladeSections sections = _get_switch_blade_sections(
                p_sampled_points, p_regular_profile, p_regular_profile, p_width, p_roll_start, p_roll_end,
                p_blade_mirrored, p_blade_sample_count);
        return _get_section_world_center(p_sampled_points, p_blade_sample_count, sections.reg_blade, p_rail_height);
    }

    double MaszynaTrackMeshBuilder::_get_blade_angle_factor(
            const PackedVector3Array &p_sampled_points, const Vector<ProfilePoint> &p_blade_profile,
            const double p_width, const double p_roll_start, const bool p_blade_mirrored, const double p_rail_height,
            const Vector3 &p_move_direction, const Vector3 &p_pivot) {
        if (p_sampled_points.is_empty()) {
            return 0.0;
        }
        const Section blade_start_section =
                _build_rail_section(p_blade_profile, p_width, p_roll_start, p_blade_mirrored);
        const Vector3 blade_start = _get_section_world_center(p_sampled_points, 0, blade_start_section, p_rail_height);
        const Vector3 per_radian_motion = UP.cross(blade_start - p_pivot);
        return p_move_direction.dot(per_radian_motion);
    }

    MaszynaTrackMeshBuilder::BladePose MaszynaTrackMeshBuilder::_build_blade_pose(
            const PackedVector3Array &p_sampled_points, const RailProfile &p_profile, const double p_width,
            const double p_roll_start, const double p_roll_end, const bool p_blade_mirrored,
            const int p_blade_sample_count, const double p_rail_height) {
        BladePose pose;
        const Vector3 move_direction = -_build_planar_parallel(p_sampled_points, 0);
        pose.pivot = _get_switch_blade_pivot(
                p_sampled_points, p_profile.rail, p_width, p_roll_start, p_roll_end, p_blade_mirrored,
                p_blade_sample_count, p_rail_height);
        pose.angle_factor = _get_blade_angle_factor(
                p_sampled_points, p_profile.blade, p_width, p_roll_start, p_blade_mirrored, p_rail_height,
                move_direction, pose.pivot);
        return pose;
    }

    Transform3D MaszynaTrackMeshBuilder::build_blade_transform(
            const Vector3 &p_origin, const BladePose &p_pose, const double p_offset) {
        if (Math::is_zero_approx(p_pose.angle_factor)) {
            return {Basis(), p_origin};
        }
        const double angle = p_offset / p_pose.angle_factor;
        return Transform3D(Basis(UP, static_cast<real_t>(angle)), p_pose.pivot) *
               Transform3D(Basis(), p_origin - p_pose.pivot);
    }

    // Fixed rail parts are generated once; moving blades are separate mesh instances.
    // MaSzyna Track.cpp:1372-1443 separates blades from fixed rails.
    void MaszynaTrackMeshBuilder::_append_switch_fixed_rail_geometry(
            Geometry &p_geometry, const PackedVector3Array &p_sampled_points,
            const Vector<ProfilePoint> &p_regular_profile, const double p_width, const double p_roll_start,
            const double p_roll_end, const double p_tex_length, const double p_rail_height, const bool p_other_mirrored,
            const bool p_blade_mirrored, const int p_blade_sample_count) {
        const Section other_start = _build_rail_section(p_regular_profile, p_width, p_roll_start, p_other_mirrored);
        const Section other_end = _build_rail_section(p_regular_profile, p_width, p_roll_end, p_other_mirrored);
        _append_loft_geometry_range_points(
                p_geometry, p_sampled_points, other_start, other_end, p_tex_length, p_rail_height, 0, CURVE_END);

        const BladeSections sections = _get_switch_blade_sections(
                p_sampled_points, p_regular_profile, p_regular_profile, p_width, p_roll_start, p_roll_end,
                p_blade_mirrored, p_blade_sample_count);
        _append_loft_geometry_range_points(
                p_geometry, p_sampled_points, sections.reg_blade, sections.reg_end, p_tex_length, p_rail_height,
                p_blade_sample_count, CURVE_END);
    }

    // The blade mesh is built at zero offset; domain fOffsetX is applied as instance transform.
    // MaSzyna Track.cpp:1975-2007 updates blade geometry only.
    // MaSzyna Segment.cpp:376-524 applies fOffsetX laterally.
    void MaszynaTrackMeshBuilder::_append_switch_blade_geometry(
            Geometry &p_geometry, const PackedVector3Array &p_sampled_points,
            const Vector<ProfilePoint> &p_regular_profile, const Vector<ProfilePoint> &p_blade_profile,
            const double p_width, const double p_roll_start, const double p_roll_end, const double p_tex_length,
            const double p_rail_height, const bool p_blade_mirrored, const int p_blade_sample_count) {
        const BladeSections sections = _get_switch_blade_sections(
                p_sampled_points, p_regular_profile, p_blade_profile, p_width, p_roll_start, p_roll_end,
                p_blade_mirrored, p_blade_sample_count);
        _append_loft_geometry_range_points(
                p_geometry, p_sampled_points, sections.blade_start, sections.reg_trans, p_tex_length, p_rail_height, 0,
                sections.transition_sample_count);
        _append_loft_geometry_range_points(
                p_geometry, p_sampled_points, sections.reg_trans, sections.reg_blade, p_tex_length, p_rail_height,
                sections.transition_sample_count, p_blade_sample_count);
    }

    // Port of the transition branch of TSegment::RenderLoft from Segment.cpp:376-524.
    Vector<MaszynaTrackMeshBuilder::StripChunk> MaszynaTrackMeshBuilder::_build_transition_loft_strip_chunks(
            const Ref<Curve3D> &p_curve, const Section &p_start_section, const Section &p_end_section,
            const double p_tex_length, const double p_rail_height) {
        Vector<StripChunk> chunks;
        if (p_start_section.size() < TRACKBED_SECTION_POINT_COUNT || p_start_section.size() != p_end_section.size()) {
            return chunks;
        }

        // Port of fixed switch segmentation from Segment.cpp:128-140.
        const PackedVector3Array sampled_points =
                p_curve->get_baked_length() > 0.0
                        ? _sample_curve_points_range(p_curve, 0.0, p_curve->get_baked_length(), SWITCH_SEGMENT_COUNT)
                        : PackedVector3Array();
        if (sampled_points.size() < 2) {
            return chunks;
        }

        const double safe_tex_length = MAX(Math::abs(p_tex_length), MIN_LENGTH);
        const int segment_count = static_cast<int>(sampled_points.size()) - 1;
        double tv1 = TEXTURE_V_RANGE;
        const Vector3 lift(0.0, static_cast<real_t>(p_rail_height), 0.0);

        for (int segment_index = 0; segment_index < segment_count; segment_index++) {
            // Port note for Segment.cpp:398-399 and Track.cpp:644-646 / 708-710:
            // original switch segments used by RenderLoft are already lifted by railheight.
            const Vector3 pos1 = sampled_points[segment_index] + lift;
            const Vector3 pos2 = sampled_points[segment_index + 1] + lift;
            // to the left, as every loft here maps the profile's x (_build_rail_section())
            const Vector3 parallel1 = -_build_planar_parallel(sampled_points, segment_index);
            const Vector3 parallel2 = -_build_planar_parallel(sampled_points, segment_index + 1);
            const double m1 = static_cast<double>(segment_index) / static_cast<double>(segment_count);
            const double m2 = static_cast<double>(segment_index + 1) / static_cast<double>(segment_count);
            const double jmm1 = 1.0 - m1;
            const double jmm2 = 1.0 - m2;
            const double segment_length = pos1.distance_to(pos2);
            // clamp_circular() of utilities.h:277
            tv1 = Math::fmod(tv1, TEXTURE_V_RANGE);
            if (tv1 < 0.0) {
                tv1 += TEXTURE_V_RANGE;
            }
            const double tv2 = tv1 - (segment_length / safe_tex_length);
            StripChunk chunk;

            // Port of per-segment strip vertex generation from Segment.cpp:452-492.
            for (int point_index = 0; point_index < TRACKBED_SECTION_POINT_COUNT; point_index++) {
                const SectionPoint &start_point = p_start_section[point_index];
                const SectionPoint &end_point = p_end_section[point_index];
                const double start_x = (jmm1 * start_point.position.x) + (m1 * end_point.position.x);
                const double start_y = (jmm1 * start_point.position.y) + (m1 * end_point.position.y);
                const double end_x = (jmm2 * start_point.position.x) + (m2 * end_point.position.x);
                const double end_y = (jmm2 * start_point.position.y) + (m2 * end_point.position.y);
                const double start_normal_x = (jmm1 * start_point.normal.x) + (m1 * end_point.normal.x);
                const double start_normal_y = (jmm1 * start_point.normal.y) + (m1 * end_point.normal.y);
                const double end_normal_x = (jmm2 * start_point.normal.x) + (m2 * end_point.normal.x);
                const double end_normal_y = (jmm2 * start_point.normal.y) + (m2 * end_point.normal.y);
                const double uv_x_start = (jmm1 * start_point.u) + (m1 * end_point.u);
                const double uv_x_end = (jmm2 * start_point.u) + (m2 * end_point.u);

                chunk.push_back(
                        {pos1 + parallel1 * static_cast<real_t>(start_x) +
                                 Vector3(0.0, static_cast<real_t>(start_y), 0.0),
                         (parallel1 * static_cast<real_t>(start_normal_x) +
                          Vector3(0.0, static_cast<real_t>(start_normal_y), 0.0))
                                 .normalized(),
                         Vector2(static_cast<real_t>(uv_x_start), static_cast<real_t>(tv1))});
                chunk.push_back(
                        {pos2 + parallel2 * static_cast<real_t>(end_x) + Vector3(0.0, static_cast<real_t>(end_y), 0.0),
                         (parallel2 * static_cast<real_t>(end_normal_x) +
                          Vector3(0.0, static_cast<real_t>(end_normal_y), 0.0))
                                 .normalized(),
                         Vector2(static_cast<real_t>(uv_x_end), static_cast<real_t>(tv2))});
            }

            chunks.push_back(chunk);
            tv1 = tv2;
        }
        return chunks;
    }

    double MaszynaTrackMeshBuilder::_find_switch_frog_distance(
            const Ref<Curve3D> &p_curve1, const Ref<Curve3D> &p_curve2, const double p_width) {
        const double length = MIN(p_curve1->get_baked_length(), p_curve2->get_baked_length());
        double dist = 0.0;
        while (dist < length) {
            const Vector3 p1 = p_curve1->sample_baked(static_cast<real_t>(dist));
            const Vector3 p2 = p_curve2->sample_baked(static_cast<real_t>(dist));
            if (p1.distance_to(p2) >= p_width) {
                return dist;
            }
            dist += FROG_SEARCH_STEP;
        }
        return length;
    }

    PackedVector3Array MaszynaTrackMeshBuilder::_sample_curve_points_range(
            const Ref<Curve3D> &p_curve, const double p_start_dist, const double p_end_dist,
            const int p_segment_count) {
        PackedVector3Array points;
        const double length = p_end_dist - p_start_dist;
        if (length <= 0.0) {
            return points;
        }
        for (int i = 0; i <= p_segment_count; i++) {
            const double d = p_start_dist + (length * static_cast<double>(i) / static_cast<double>(p_segment_count));
            points.push_back(p_curve->sample_baked(static_cast<real_t>(d), true));
        }
        return points;
    }

    PackedVector3Array
    MaszynaTrackMeshBuilder::_sample_switch_curve_points(const Ref<Curve3D> &p_curve, const double p_frog_dist) {
        PackedVector3Array points = _sample_curve_points_range(p_curve, 0.0, p_frog_dist, SWITCH_SEGMENT_COUNT);
        const double total_length = p_curve->get_baked_length();
        if (total_length > p_frog_dist + SWITCH_TAIL_MIN_LENGTH) {
            const int rem_segments =
                    MAX(1, static_cast<int>((total_length - p_frog_dist) / SWITCH_TAIL_SEGMENT_LENGTH));
            const PackedVector3Array rem_points =
                    _sample_curve_points_range(p_curve, p_frog_dist, total_length, rem_segments);
            for (int i = 1; i < rem_points.size(); i++) {
                points.push_back(rem_points[i]);
            }
        }
        return points;
    }

    // Port of planar cross-track vector construction from Segment.cpp:398-405 and Segment.cpp:444-450.
    Vector3
    MaszynaTrackMeshBuilder::_build_planar_parallel(const PackedVector3Array &p_points, const int p_sample_index) {
        const int last = static_cast<int>(p_points.size()) - 1;
        Vector3 tangent;
        if (p_sample_index <= 0) {
            tangent = p_points[1] - p_points[0];
        } else if (p_sample_index >= last) {
            tangent = p_points[last] - p_points[last - 1];
        } else {
            tangent = p_points[p_sample_index + 1] - p_points[p_sample_index - 1];
        }

        Vector3 parallel(-tangent.z, 0.0, tangent.x);
        if (parallel.length_squared() <= MIN_DIRECTION_LENGTH_SQUARED) {
            parallel = p_points[last] - p_points[0];
            parallel = Vector3(-parallel.z, 0.0, parallel.x);
        }
        if (parallel.length_squared() <= MIN_DIRECTION_LENGTH_SQUARED) {
            return RIGHT;
        }
        return parallel.normalized();
    }

    double MaszynaTrackMeshBuilder::_get_baked_segment_length(
            const PackedVector3Array &p_points, const int p_start_sample, const int p_end_sample) {
        double length = 0.0;
        const int last = p_end_sample < 0 ? static_cast<int>(p_points.size()) - 1 : p_end_sample;
        for (int sample_index = p_start_sample; sample_index < last; sample_index++) {
            length += p_points[sample_index].distance_to(p_points[sample_index + 1]);
        }
        return length;
    }

    Transform3D
    MaszynaTrackMeshBuilder::_build_curve_frame(const PackedVector3Array &p_points, const int p_sample_index) {
        const Vector3 origin = p_points[p_sample_index];
        Vector3 tangent;
        if (p_sample_index == 0) {
            tangent = p_points[1] - p_points[0];
        } else if (p_sample_index == p_points.size() - 1) {
            tangent = p_points[p_sample_index] - p_points[p_sample_index - 1];
        } else {
            tangent = p_points[p_sample_index + 1] - p_points[p_sample_index - 1];
        }

        if (tangent.length_squared() <= MIN_DIRECTION_LENGTH_SQUARED) {
            tangent = FORWARD;
        } else {
            tangent = tangent.normalized();
        }

        Vector3 reference_up = UP;
        if (Math::abs(tangent.dot(reference_up)) > VERTICAL_TANGENT_DOT) {
            reference_up = RIGHT;
        }

        const Vector3 right = reference_up.cross(tangent).normalized();
        const Vector3 up = tangent.cross(right).normalized();
        return {Basis(right, up, tangent).orthonormalized(), origin};
    }

    PackedVector3Array
    MaszynaTrackMeshBuilder::_vertices_relative_to(const PackedVector3Array &p_vertices, const Vector3 &p_origin) {
        PackedVector3Array vertices = p_vertices;
        for (Vector3 &vertex: vertices) {
            vertex -= p_origin;
        }
        return vertices;
    }

    Array MaszynaTrackMeshBuilder::build_surface_arrays(const Geometry &p_geometry, const Vector3 &p_origin) {
        Array arrays;
        arrays.resize(Mesh::ARRAY_MAX);
        arrays[Mesh::ARRAY_VERTEX] = _vertices_relative_to(p_geometry.vertices, p_origin);
        arrays[Mesh::ARRAY_NORMAL] = p_geometry.normals;
        arrays[Mesh::ARRAY_TEX_UV] = p_geometry.uvs;
        arrays[Mesh::ARRAY_INDEX] = p_geometry.indices;
        return arrays;
    }

    void MaszynaTrackMeshBuilder::append_geometry(Geometry &p_geometry, const Geometry &p_source) {
        const int base_vertex = static_cast<int>(p_geometry.vertices.size());
        p_geometry.vertices.append_array(p_source.vertices);
        p_geometry.normals.append_array(p_source.normals);
        p_geometry.uvs.append_array(p_source.uvs);
        for (const int32_t index: p_source.indices) {
            p_geometry.indices.push_back(base_vertex + index);
        }
    }

    /// Port of TTrack::create_road_profile() (Track.cpp:3008-3067) at one end of the road: its left
    /// and right edges in the original's frame (x to the right of the road's direction), the texture
    /// cut to the road's width - a texture is as wide as its length times its width/height ratio
    MaszynaTrackMeshBuilder::Section MaszynaTrackMeshBuilder::build_road_section(
            const double p_width, const double p_roll, const double p_texture_length, const double p_ratio) {
        const double half_width = 0.5 * Math::abs(p_width);
        const double texture_width = p_ratio * p_texture_length;
        const double map = texture_width > 0.0 ? half_width / texture_width : ROAD_DEFAULT_MAP;
        const Vector3 left(
                static_cast<real_t>(half_width * Math::cos(p_roll)),
                static_cast<real_t>(-half_width * Math::sin(p_roll)), 0.0);
        Section section;
        section.push_back({left, UP, 0.5 - map});
        section.push_back({-left, UP, 0.5 + map});
        return section;
    }

    /// Port of TTrack::create_road_side_profile() (Track.cpp:3069-3256) at one end of the road: right
    /// and left, each a section in the original's frame. A non-negative texture height is a shoulder:
    /// flat for tex_width, then down to -tex_height over tex_slope. A negative one is a sidewalk: a curb
    /// as high, its face sloped 1:3.75, then a sidewalk tex_slope wide on the right and tex_width wide
    /// on the left, mapped in proportion to its width.
    void MaszynaTrackMeshBuilder::build_road_side_sections(
            const RoadLook &p_road, const Section &p_surface, const double p_texture_length, const double p_ratio,
            Section &p_r_right, Section &p_r_left) {
        const double half_width = 0.5 * Math::abs(p_road.width);
        const double side = Math::abs(p_road.tex_width);
        const double slope = Math::abs(p_road.tex_slope);
        const Vector3 road_left = p_surface[0].position;
        const Vector3 road_right = p_surface[1].position;
        p_r_right.clear();
        p_r_left.clear();
        if (p_road.tex_height >= 0.0) {
            const double outer = half_width + side + slope;
            p_r_right.push_back(
                    {Vector3(static_cast<real_t>(outer), static_cast<real_t>(-p_road.tex_height), 0.0), RIGHT, 0.0});
            p_r_right.push_back({Vector3(road_left.x + static_cast<real_t>(side), road_left.y, 0.0), UP, 0.5});
            p_r_right.push_back({road_left, UP, 1.0});
            p_r_left.push_back({road_right, UP, 1.0});
            p_r_left.push_back({Vector3(road_right.x - static_cast<real_t>(side), road_right.y, 0.0), UP, 0.5});
            p_r_left.push_back(
                    {Vector3(static_cast<real_t>(-outer), static_cast<real_t>(-p_road.tex_height), 0.0), LEFT, 0.0});
            return;
        }
        // a curb 150 mm high leans 40 mm; the curbs are mapped from 31/64 to 32/64 and 32/64 to 33/64
        const double lean = -p_road.tex_height / CURB_SLOPE;
        const double texture_width = p_ratio * p_texture_length;
        const double map_left = texture_width > 0.0 ? side / texture_width : CURB_LEFT_U;
        const double map_right = texture_width > 0.0 ? slope / texture_width : CURB_LEFT_U;
        const double height_right = slope > lean ? -p_road.tex_height : 0.0;
        const double height_left = side > lean ? -p_road.tex_height : 0.0;
        p_r_right.push_back(
                {Vector3(road_left.x + static_cast<real_t>(slope), road_left.y + static_cast<real_t>(height_right),
                         0.0),
                 UP, CURB_RIGHT_U + map_right});
        p_r_right.push_back(
                {Vector3(road_left.x + static_cast<real_t>(lean), road_left.y + static_cast<real_t>(height_right), 0.0),
                 UP, CURB_RIGHT_U});
        p_r_right.push_back({road_left, LEFT, CURB_RIGHT_U - (lean / CURB_LEAN_PER_U)});
        p_r_left.push_back({road_right, RIGHT, CURB_LEFT_U + (lean / CURB_LEAN_PER_U)});
        p_r_left.push_back(
                {Vector3(road_right.x - static_cast<real_t>(lean), road_right.y + static_cast<real_t>(height_left),
                         0.0),
                 UP, CURB_LEFT_U});
        p_r_left.push_back(
                {Vector3(road_right.x - static_cast<real_t>(side), road_right.y + static_cast<real_t>(height_left),
                         0.0),
                 UP, CURB_LEFT_U - map_left});
    }

    /// A section of the original's frame (x to the right of the road's direction) in this backend's
    /// loft frame, which has x to the left (_build_rail_section()): mirrored, and reversed so its faces
    /// still face up
    MaszynaTrackMeshBuilder::Section MaszynaTrackMeshBuilder::mirror_road_section(const Section &p_section) {
        Section mirrored;
        for (const SectionPoint &point: p_section) {
            mirrored.insert(
                    0, {Vector3(-point.position.x, point.position.y, 0.0),
                        Vector3(-point.normal.x, point.normal.y, 0.0), point.u});
        }
        return mirrored;
    }

    // Port of switch trackbed generation and assembly from Track.cpp:3236-3302.
    void MaszynaTrackMeshBuilder::_append_switch_trackbed_geometry(
            Geometry &p_geometry, const TrackPiece &p_piece, const Ref<Curve3D> &p_primary_curve,
            const Ref<Curve3D> &p_secondary_curve) {
        // Port of per-branch loft generation from Track.cpp:3250-3259.
        const Vector<StripChunk> primary_chunks = _build_transition_loft_strip_chunks(
                p_primary_curve, p_piece.bed_start, p_piece.bed_end, p_piece.bed_texture_length, p_piece.rail_height);
        const Vector<StripChunk> secondary_chunks = _build_transition_loft_strip_chunks(
                p_secondary_curve, p_piece.second_bed_start, p_piece.second_bed_end, p_piece.bed_texture_length,
                p_piece.rail_height);

        const int segment_count = static_cast<int>(MIN(primary_chunks.size(), secondary_chunks.size()));

        for (int segment_index = 0; segment_index < segment_count; segment_index++) {
            StripChunk primary_chunk = primary_chunks[segment_index];
            StripChunk secondary_chunk = secondary_chunks[segment_index];

            // Port of samplersoffset-based edge lowering from Track.cpp:3265-3299.
            // The left edge is the profile's +x edge, to the left of the track
            // (_build_transition_loft_strip_chunks())
            const int primary_fallback_edge =
                    p_piece.switch_is_right ? TRACKBED_RIGHT_EDGE_POINT : TRACKBED_LEFT_EDGE_POINT;
            const int secondary_fallback_edge =
                    p_piece.switch_is_right ? TRACKBED_LEFT_EDGE_POINT : TRACKBED_RIGHT_EDGE_POINT;
            const int primary_lower_edge =
                    _nearest_strip_inner_edge(primary_chunk, secondary_chunk, primary_fallback_edge);
            const int secondary_lower_edge =
                    _nearest_strip_inner_edge(secondary_chunk, primary_chunk, secondary_fallback_edge);
            _lower_strip_edge(primary_chunk, primary_lower_edge);
            _lower_strip_edge(secondary_chunk, secondary_lower_edge);

            // Port of the per-branch output order from Track.cpp:3280-3297.
            // Segment.cpp:452-492 emits one 10-vertex strip chunk at a time; only that chunk
            // describes loft faces. Triangulating across chunk boundaries exposes strip
            // connector triangles as visible geometry in Godot.
            _append_strip_chunk(p_geometry, primary_chunk);
            _append_strip_chunk(p_geometry, secondary_chunk);
        }
    }

    // Port of vertex lowering index selection used during switch trackbed assembly from
    // Track.cpp:3271-3293: both vertices of the section point p_edge_point go down.
    void MaszynaTrackMeshBuilder::_lower_strip_edge(StripChunk &p_chunk, const int p_edge_point) {
        for (int vertex = 0; vertex < STRIP_VERTICES_PER_POINT; vertex++) {
            const int vertex_index = (p_edge_point * STRIP_VERTICES_PER_POINT) + vertex;
            if (vertex_index < 0 || vertex_index >= p_chunk.size()) {
                continue;
            }
            p_chunk.write[vertex_index].position.y -= static_cast<real_t>(SWITCH_TRACKBED_Z_FIGHT_OFFSET);
        }
    }

    /// The edge of p_chunk (a section point of its upper slope edges) nearer to the middle of
    /// p_other_chunk, across the ground; p_fallback_edge when the two are as near
    int MaszynaTrackMeshBuilder::_nearest_strip_inner_edge(
            const StripChunk &p_chunk, const StripChunk &p_other_chunk, const int p_fallback_edge) {
        constexpr int RIGHT_EDGE_END = (TRACKBED_RIGHT_EDGE_POINT + 1) * STRIP_VERTICES_PER_POINT;
        constexpr int CENTRE_END = (TRACKBED_CENTRE_POINT + 1) * STRIP_VERTICES_PER_POINT;
        if (p_chunk.size() < RIGHT_EDGE_END || p_other_chunk.size() < CENTRE_END) {
            return p_fallback_edge;
        }
        const auto edge_center = [](const StripChunk &p_strip, const int p_point) {
            const int first = p_point * STRIP_VERTICES_PER_POINT;
            return (p_strip[first].position + p_strip[first + 1].position) * 0.5;
        };
        const Vector3 other_center = edge_center(p_other_chunk, TRACKBED_CENTRE_POINT);
        Vector3 left_delta = edge_center(p_chunk, TRACKBED_LEFT_EDGE_POINT) - other_center;
        Vector3 right_delta = edge_center(p_chunk, TRACKBED_RIGHT_EDGE_POINT) - other_center;
        left_delta.y = 0.0;
        right_delta.y = 0.0;

        const double left_distance = left_delta.length_squared();
        const double right_distance = right_delta.length_squared();
        if (Math::is_equal_approx(left_distance, right_distance)) {
            return p_fallback_edge;
        }
        return left_distance < right_distance ? TRACKBED_LEFT_EDGE_POINT : TRACKBED_RIGHT_EDGE_POINT;
    }

    // Port of strip vertex copying from Track.cpp:3280-3297, and of the visible strip faces emitted per
    // loft chunk by Segment.cpp:452-492. Wound like _append_loft_strip_indices(): the chunk maps the
    // profile's x to the left, so the original's order faces up here without reversing it.
    void MaszynaTrackMeshBuilder::_append_strip_chunk(Geometry &p_geometry, const StripChunk &p_chunk) {
        const int base_vertex = static_cast<int>(p_geometry.vertices.size());
        for (const StripPoint &point: p_chunk) {
            p_geometry.vertices.push_back(point.position);
            p_geometry.normals.push_back(point.normal);
            p_geometry.uvs.push_back(point.uv);
        }
        for (int point_index = 0; point_index < p_chunk.size() - 2; point_index++) {
            const int a = base_vertex + point_index;
            const int b = base_vertex + point_index + 1;
            const int c = base_vertex + point_index + 2;
            if (point_index % 2 == 0) {
                p_geometry.indices.push_back(a);
                p_geometry.indices.push_back(b);
            } else {
                p_geometry.indices.push_back(b);
                p_geometry.indices.push_back(a);
            }
            p_geometry.indices.push_back(c);
        }
    }

    MaszynaTrackMeshBuilder::PieceGeometry MaszynaTrackMeshBuilder::build_piece_geometry(const TrackPiece &p_piece) {
        PieceGeometry geometry;
        const Ref<Curve3D> curve1 = _build_curve(p_piece.curve1);
        if (curve1->get_baked_length() == 0.0) {
            return geometry;
        }
        const PackedVector3Array curve1_points = curve1->get_baked_points();
        if (p_piece.shape == PIECE_ROAD) {
            _append_loft_geometry_range_points(
                    geometry.primary_rails, curve1_points, p_piece.surface_start, p_piece.surface_end,
                    p_piece.primary_texture_length, 0.0, 0, CURVE_END);
            _append_loft_geometry_range_points(
                    geometry.bed, curve1_points, p_piece.right_start, p_piece.right_end, p_piece.bed_texture_length,
                    0.0, 0, CURVE_END);
            _append_loft_geometry_range_points(
                    geometry.bed, curve1_points, p_piece.left_start, p_piece.left_end, p_piece.bed_texture_length, 0.0,
                    0, CURVE_END);
            return geometry;
        }

        const CurveShape &curve1_data = p_piece.curve1;
        const CurveShape &curve2_data = p_piece.curve2;
        const Vector<ProfilePoint> &regular_profile = p_piece.rail_profile.rail;
        const double track_width = p_piece.width;
        const double rail_height = p_piece.rail_height;
        const double tex_length = p_piece.primary_texture_length;

        if (p_piece.shape == PIECE_SWITCH) {
            const Ref<Curve3D> curve2 = _build_curve(curve2_data);
            const Vector<ProfilePoint> &blade_profile = p_piece.rail_profile.blade;
            // MaSzyna Track.cpp:1367-1453 emits switch rail segments.
            const double frog_dist = _find_switch_frog_distance(curve1, curve2, track_width);
            const PackedVector3Array primary_sampled_points = _sample_switch_curve_points(curve1, frog_dist);
            const PackedVector3Array secondary_sampled_points = _sample_switch_curve_points(curve2, frog_dist);
            const int blade_sample_count =
                    static_cast<int>(Math::ceil(static_cast<double>(SWITCH_SEGMENT_COUNT) * SWITCH_BLADE_RATIO));
            // the blades are moved by their instances, the piece's layout says only which rails they are
            const SwitchBladeLayout &layout = p_piece.blade_layout;

            _append_switch_fixed_rail_geometry(
                    geometry.primary_rails, primary_sampled_points, regular_profile, track_width, curve1_data.roll1,
                    curve1_data.roll2, tex_length, rail_height, layout.primary_other_mirrored,
                    layout.primary_blade_mirrored, blade_sample_count);
            _append_switch_fixed_rail_geometry(
                    geometry.secondary_rails, secondary_sampled_points, regular_profile, track_width, curve2_data.roll1,
                    curve2_data.roll2, tex_length, rail_height, layout.secondary_other_mirrored,
                    layout.secondary_blade_mirrored, blade_sample_count);
            _append_switch_blade_geometry(
                    geometry.primary_blade, primary_sampled_points, regular_profile, blade_profile, track_width,
                    curve1_data.roll1, curve1_data.roll2, tex_length, rail_height, layout.primary_blade_mirrored,
                    blade_sample_count);
            _append_switch_blade_geometry(
                    geometry.secondary_blade, secondary_sampled_points, regular_profile, blade_profile, track_width,
                    curve2_data.roll1, curve2_data.roll2, tex_length, rail_height, layout.secondary_blade_mirrored,
                    blade_sample_count);
            geometry.primary_blade_pose = _build_blade_pose(
                    primary_sampled_points, p_piece.rail_profile, track_width, curve1_data.roll1, curve1_data.roll2,
                    layout.primary_blade_mirrored, blade_sample_count, rail_height);
            geometry.secondary_blade_pose = _build_blade_pose(
                    secondary_sampled_points, p_piece.rail_profile, track_width, curve2_data.roll1, curve2_data.roll2,
                    layout.secondary_blade_mirrored, blade_sample_count, rail_height);
            if (!p_piece.bed_start.is_empty()) {
                _append_switch_trackbed_geometry(geometry.bed, p_piece, curve1, curve2);
            }
            return geometry;
        }

        const Section right_start = _build_rail_section(regular_profile, track_width, curve1_data.roll1, false);
        const Section right_end = _build_rail_section(regular_profile, track_width, curve1_data.roll2, false);
        const Section left_start = _build_rail_section(regular_profile, track_width, curve1_data.roll1, true);
        const Section left_end = _build_rail_section(regular_profile, track_width, curve1_data.roll2, true);
        _append_loft_geometry_range_points(
                geometry.primary_rails, curve1_points, right_start, right_end, tex_length, rail_height, 0, CURVE_END);
        _append_loft_geometry_range_points(
                geometry.primary_rails, curve1_points, left_start, left_end, tex_length, rail_height, 0, CURVE_END);
        _append_loft_geometry_range_points(
                geometry.bed, curve1_points, p_piece.bed_start, p_piece.bed_end, p_piece.bed_texture_length,
                rail_height, 0, CURVE_END);

        if (p_piece.has_curve2) {
            const PackedVector3Array secondary_points = _build_curve(curve2_data)->get_baked_points();
            const Section second_right_start =
                    _build_rail_section(regular_profile, track_width, curve2_data.roll1, false);
            const Section second_right_end =
                    _build_rail_section(regular_profile, track_width, curve2_data.roll2, false);
            const Section second_left_start =
                    _build_rail_section(regular_profile, track_width, curve2_data.roll1, true);
            const Section second_left_end = _build_rail_section(regular_profile, track_width, curve2_data.roll2, true);
            _append_loft_geometry_range_points(
                    geometry.secondary_rails, secondary_points, second_right_start, second_right_end, tex_length,
                    rail_height, 0, CURVE_END);
            _append_loft_geometry_range_points(
                    geometry.secondary_rails, secondary_points, second_left_start, second_left_end, tex_length,
                    rail_height, 0, CURVE_END);
            _append_loft_geometry_range_points(
                    geometry.bed, secondary_points, p_piece.second_bed_start, p_piece.second_bed_end,
                    p_piece.bed_texture_length, rail_height, 0, CURVE_END);
        }
        return geometry;
    }
} // namespace godot
