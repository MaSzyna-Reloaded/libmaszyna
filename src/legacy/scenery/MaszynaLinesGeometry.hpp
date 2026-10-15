#pragma once

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <vector>

namespace godot {
    /// The scenery's "lines", "line_strip" and "line_loop" nodes of one section of the 1 km grid -
    /// the wires of power lines, mostly - merged as the original merges them in its cells
    /// (basic_cell::insert(), scene.cpp:302-326; lines_node::merge(), scenenode.cpp:653-672), and
    /// made into a mesh drawn as the original draws them (opengl33renderer.cpp:4132-4163): a line a
    /// few pixels wide, fading with the distance from its node's centre (scenery_lines.gdshader).
    ///
    /// A node's lines are pairs of points - a strip or a loop is turned into pairs as it is read
    /// (lines_node::import(), scenenode.cpp:604-642). Positions are kept in the world and given
    /// to the mesh relative to the origin, the section's centre.
    class MaszynaLinesGeometry : public RefCounted {
            GDCLASS(MaszynaLinesGeometry, RefCounted)

        public:
            /// The original's cell the nodes merge in (scene.h:34, EU07_CELLSIZE) and its section
            /// (scene.h:35, EU07_SECTIONSIZE)
            static constexpr double CELL_SIZE_M = 250.0;
            static constexpr double SECTION_SIZE_M = 1000.0;
            /// Nodes merge only when their centres are closer than this (scene.cpp:315)
            static constexpr double MERGE_DISTANCE_M = 10.0;

        private:
            /// A node, or the nodes merged into it
            struct LinesNode {
                    Color color;
                    float width = 0.0F;
                    float range_min = 0.0F;
                    float range_max = 0.0F;
                    /// The mean of its points (the weighted mix of lines_node::merge() is that mean)
                    Vector3 centre;
                    /// The farthest point from the centre (lines_node::compute_radius(), scenenode.cpp:692)
                    float radius = 0.0F;
                    /// The cell of the section it is kept in (basic_section::cell(), scene.cpp:1028)
                    int cell = 0;
                    std::vector<Vector3> points;
            };

            Vector3 origin;
            std::vector<LinesNode> nodes;

            int _get_cell(const Vector3 &p_position) const;

        protected:
            static void _bind_methods();

        public:
            /// The vertices of a line's quad, and the indices of its two triangles
            static constexpr int QUAD_VERTICES = 4;
            static constexpr int QUAD_INDICES = 6;

            /* The centre of the section of the 1 km grid holding p_position - the origin of the
             * geometry of the lines whose centre is there (basic_region::section(), scene.cpp:1723) */
            static Vector3 position_get_section_origin(const Vector3 &p_position);

            void set_origin(const Vector3 &p_origin);
            Vector3 get_origin() const;

            /* A node's lines in the world, pairs of points (an odd last one is left out); its
             * range_max 0 is no limit. Merged into a node of its cell with the same ranges, colour
             * and width whose centre is closer than MERGE_DISTANCE_M, as the original does */
            void add_lines(
                    const PackedVector3Array &p_points, const Color &p_color, float p_width, float p_range_min,
                    float p_range_max);
            int64_t get_node_count() const;
            /* A quad per line, relative to the origin: VERTEX one end, CUSTOM0 the other end and the
             * side, CUSTOM1 the node's centre and radius, CUSTOM2 its width, range_min and range_max,
             * COLOR its colour - what scenery_lines.gdshader draws it from. No material */
            Ref<ArrayMesh> build_mesh() const;
            /* How far from the mesh's centre the camera sees a line of it - 0 when one has no limit */
            float get_range_end() const;
    };
} // namespace godot
