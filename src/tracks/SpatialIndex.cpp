#include "SpatialIndex.hpp"

#include <godot_cpp/core/class_db.hpp>

namespace godot {
    void SpatialIndex::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_cell_size", "cell_size"), &SpatialIndex::set_cell_size);
        ClassDB::bind_method(D_METHOD("get_cell_size"), &SpatialIndex::get_cell_size);
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "cell_size"), "set_cell_size", "get_cell_size");

        ClassDB::bind_method(D_METHOD("add", "item", "aabb"), &SpatialIndex::add);
        ClassDB::bind_method(D_METHOD("remove", "item"), &SpatialIndex::remove);
        ClassDB::bind_method(D_METHOD("query", "aabb"), &SpatialIndex::query);
        ClassDB::bind_method(D_METHOD("clear"), &SpatialIndex::clear);
    }

    void SpatialIndex::set_cell_size(const double p_cell_size) {
        ERR_FAIL_COND_MSG(p_cell_size <= 0.0, "SpatialIndex cell size must be positive");
        cell_size = p_cell_size;
    }

    double SpatialIndex::get_cell_size() const {
        return cell_size;
    }

    void SpatialIndex::_cells_for_aabb(const Rect2 &p_aabb, Vector<Vector2i> &p_cells) const {
        const real_t size = static_cast<real_t>(cell_size);
        const Vector2 min_cell = (p_aabb.position / size).floor();
        const Vector2 max_cell = ((p_aabb.position + p_aabb.size) / size).floor();
        for (int x = static_cast<int>(min_cell.x); x <= static_cast<int>(max_cell.x); ++x) {
            for (int y = static_cast<int>(min_cell.y); y <= static_cast<int>(max_cell.y); ++y) {
                p_cells.push_back(Vector2i(x, y));
            }
        }
    }

    void SpatialIndex::add(const RID &p_item, const Rect2 &p_aabb) {
        Vector<Vector2i> touched;
        _cells_for_aabb(p_aabb, touched);
        for (const Vector2i &cell: touched) {
            cells[cell].push_back(p_item);
            item_cells[p_item].push_back(cell);
        }
    }

    void SpatialIndex::remove(const RID &p_item) {
        const Vector<Vector2i> *touched = item_cells.getptr(p_item);
        if (touched == nullptr) {
            return;
        }
        for (const Vector2i &cell: *touched) {
            if (Vector<RID> *items = cells.getptr(cell); items != nullptr) {
                items->erase(p_item);
            }
        }
        item_cells.erase(p_item);
    }

    TypedArray<RID> SpatialIndex::query(const Rect2 &p_aabb) const {
        Vector<Vector2i> touched;
        _cells_for_aabb(p_aabb, touched);
        // an item filed in several touched cells must come back once
        HashMap<RID, bool> seen;
        TypedArray<RID> result;
        for (const Vector2i &cell: touched) {
            const Vector<RID> *items = cells.getptr(cell);
            if (items == nullptr) {
                continue;
            }
            for (const RID &item: *items) {
                if (seen.has(item)) {
                    continue;
                }
                seen.insert(item, true);
                result.push_back(item);
            }
        }
        return result;
    }

    void SpatialIndex::clear() {
        cells.clear();
        item_cells.clear();
    }
} // namespace godot
