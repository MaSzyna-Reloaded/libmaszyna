#pragma once

#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/templates/hashfuncs.hpp>
#include <godot_cpp/variant/rid.hpp>

#include <cstdint>
#include <cstring>

namespace godot {
    /// The engine's RID is its 64-bit id and nothing else (core/templates/rid.h)
    static_assert(sizeof(RID) == sizeof(uint64_t));

    /// A RID's id read from its bytes. godot-cpp's RID::get_id() and RID::operator== are calls into
    /// the engine - a builtin method and a variant operator - and a HashMap keyed by RID makes one
    /// on every hash and every compare, several per lookup in the simulation's inner loops.
    _FORCE_INLINE_ uint64_t rid_get_id(const RID &p_rid) {
        uint64_t id = 0;
        memcpy(&id, p_rid._native_ptr(), sizeof(id));
        return id;
    }

    struct RIDHasher {
            static _FORCE_INLINE_ uint32_t hash(const RID &p_rid) {
                return hash_one_uint64(rid_get_id(p_rid));
            }
    };

    struct RIDComparator {
            static _FORCE_INLINE_ bool compare(const RID &p_lhs, const RID &p_rhs) {
                return rid_get_id(p_lhs) == rid_get_id(p_rhs);
            }
    };

    /// A map and a set keyed by RID that hash and compare without calling the engine
    template<typename TValue>
    using RIDMap = HashMap<RID, TValue, RIDHasher, RIDComparator>;
    using RIDSet = HashSet<RID, RIDHasher, RIDComparator>;
} // namespace godot
