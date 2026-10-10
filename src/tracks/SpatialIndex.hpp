#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector2i.hpp>

namespace godot {
    /* A uniform grid over the XZ plane: every item is filed in each cell its AABB touches, and a
     * query returns the items of the touched cells. Shared by TrackServer and by
     * TractionServer, which is why it is a class of its own rather than a member of either. */
    class SpatialIndex : public RefCounted {
            GDCLASS(SpatialIndex, RefCounted)

        private:
            double cell_size = 1.0;
            HashMap<Vector2i, Vector<RID>> cells;
            HashMap<RID, Vector<Vector2i>> item_cells;

            void _cells_for_aabb(const Rect2 &p_aabb, Vector<Vector2i> &p_cells) const;

        protected:
            static void _bind_methods();

        public:
            /* Edge length of one grid cell. Set once, before the first item is added - changing it
             * with items filed would leave them in cells of the previous size. */
            void set_cell_size(double p_cell_size);
            double get_cell_size() const;

            void add(const RID &p_item, const Rect2 &p_aabb);
            void remove(const RID &p_item);
            TypedArray<RID> query(const Rect2 &p_aabb) const;
            void clear();
    };
} // namespace godot
