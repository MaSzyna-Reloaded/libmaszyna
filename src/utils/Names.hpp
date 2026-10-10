#pragma once
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    /// Moves an entry of a name table from one name to another. The original's name tables let the
    /// newest entry win a duplicate name (Names.h:29); an empty name leaves the entry unnamed.
    inline void names_rename(
            HashMap<StringName, RID> &p_names, const StringName &p_from, const StringName &p_to, const RID &p_rid) {
        if (const RID *named = p_names.getptr(p_from); named != nullptr && *named == p_rid) {
            p_names.erase(p_from);
        }
        if (p_to.is_empty()) {
            return;
        }
        if (p_names.has(p_to)) {
            UtilityFunctions::push_warning("Duplicate name, the last one wins: " + String(p_to));
        }
        p_names.insert(p_to, p_rid);
    }
} // namespace godot
