#ifndef MACROS_HPP
#define MACROS_HPP
#include "utils/utils.hpp"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>

#include <algorithm>
#include <initializer_list>
#include <utility>

/**
 * Builds a PROPERTY_HINT_ENUM hint string from real enum constants instead of hand-typed
 * integer literals, so a property's inspector labels can never drift out of sync with the
 * enum's actual (possibly non-sequential/bitflag) values.
 * <example>
 * enum_hint({{"Train", CATEGORY_TRAIN}, {"Road", CATEGORY_ROAD}, {"Ship", CATEGORY_SHIP}, {"Airplane",
 * CATEGORY_AIRPLANE}})
 * </example>
 */
inline godot::String enum_hint(std::initializer_list<std::pair<const char *, int64_t>> p_entries) {
    godot::String result;
    bool first = true;
    for (const auto &entry: p_entries) {
        if (!first) {
            result += ",";
        }
        result += godot::String(entry.first) + ":" + godot::String::num_int64(entry.second);
        first = false;
    }
    return result;
}
/**
 * Macro for generating private members with their setters and getters
 * @param type Member type
 * @param name Member name
 * @param default_value Default value
 */
#define MAKE_MEMBER_GS(type, name, default_value)                                                                      \
private:                                                                                                               \
    type name = default_value;                                                                                         \
                                                                                                                       \
public:                                                                                                                \
    const type &get_##name() const {                                                                                   \
        return name;                                                                                                   \
    }                                                                                                                  \
    void set_##name(const type &value) {                                                                               \
        name = value;                                                                                                  \
    }
/**
 * Macro for generating private members with their setters and getters without any default value
 * @param type Member type
 * @param name Member name
 * @param default_value Default value
 */
#define MAKE_MEMBER_GS_NO_DEF(type, name)                                                                              \
private:                                                                                                               \
    type name;                                                                                                         \
                                                                                                                       \
public:                                                                                                                \
    const type &get_##name() const {                                                                                   \
        return name;                                                                                                   \
    }                                                                                                                  \
    void set_##name(const type &value) {                                                                               \
        name = value;                                                                                                  \
    }

/**
 * Macro for generating private synchronized members with their setters and getters. Those members will ALWAYS be
 * synchronized with internal mover
 * @param type Member type
 * @param name Member name
 * @param default_value Default value
 */
#define MAKE_MEMBER_GS_DIRTY(type, name, default_value)                                                                \
private:                                                                                                               \
    type name = default_value;                                                                                         \
                                                                                                                       \
public:                                                                                                                \
    const type &get_##name() const {                                                                                   \
        return name;                                                                                                   \
    }                                                                                                                  \
    void set_##name(const type &value) {                                                                               \
        name = value;                                                                                                  \
        dirty = true;                                                                                                  \
    }

/**
 * Macro for generating private members with their setters and getters without a const reference
 * @param type Member type
 * @param name Member name
 * @param default_value Default value
 */
#define MAKE_MEMBER_GS_NR(type, name, default_value)                                                                   \
private:                                                                                                               \
    type name = default_value;                                                                                         \
                                                                                                                       \
public:                                                                                                                \
    type get_##name() const {                                                                                          \
        return name;                                                                                                   \
    }                                                                                                                  \
    void set_##name(type value) {                                                                                      \
        name = value;                                                                                                  \
    }

/**
 * Macro for generating private members with their setters and getters without a const reference
 * and without default value
 * @param type Member type
 * @param name Member name
 */

#define MAKE_MEMBER_GS_NR_NO_DEF(type, name)                                                                           \
private:                                                                                                               \
    type name;                                                                                                         \
                                                                                                                       \
public:                                                                                                                \
    type get_##name() const {                                                                                          \
        return name;                                                                                                   \
    }                                                                                                                  \
    void set_##name(type value) {                                                                                      \
        name = value;                                                                                                  \
    }

#define INTERNAL_PROPERTY_SELECT_3(_1, _2, _3, p_macro, ...) p_macro
#define INTERNAL_PROPERTY_SELECT_4(_1, _2, _3, _4, p_macro, ...) p_macro
#define INTERNAL_PROPERTY_SELECT_6(_1, _2, _3, _4, _5, _6, p_macro, ...) p_macro

#define INTERNAL_PROPERTY_STRINGIFY_EXPANDED(p_value) #p_value
#define INTERNAL_PROPERTY_STRINGIFY(p_value) INTERNAL_PROPERTY_STRINGIFY_EXPANDED(p_value)

#define INTERNAL_PROPERTY_SETTER_EXPANDED(p_name) set_##p_name
#define INTERNAL_PROPERTY_SETTER(p_name) INTERNAL_PROPERTY_SETTER_EXPANDED(p_name)
#define INTERNAL_PROPERTY_GETTER_EXPANDED(p_name) get_##p_name
#define INTERNAL_PROPERTY_GETTER(p_name) INTERNAL_PROPERTY_GETTER_EXPANDED(p_name)

namespace libmaszyna::internal {
    inline godot::String merge_property_group_segment(const godot::String &p_prefix, const godot::String &p_segment) {
        if (p_prefix.is_empty()) {
            return p_segment;
        }

        const godot::PackedStringArray prefix_words = p_prefix.split("_");
        const godot::PackedStringArray segment_words = p_segment.split("_");
        int64_t overlap = 0;
        const int64_t maximum_overlap = std::min(prefix_words.size(), segment_words.size());
        for (int64_t size = maximum_overlap; size > 0; --size) {
            bool matches = true;
            for (int64_t index = 0; index < size; ++index) {
                if (prefix_words[prefix_words.size() - size + index] != segment_words[index]) {
                    matches = false;
                    break;
                }
            }
            if (matches) {
                overlap = size;
                break;
            }
        }

        godot::String result = p_prefix;
        for (int64_t index = overlap; index < segment_words.size(); ++index) {
            result += "_" + segment_words[index];
        }
        return result;
    }

    inline void add_property_groups(const godot::StringName &p_class, const godot::String &p_path) {
        const godot::PackedStringArray groups = p_path.split("/");
        godot::String prefix;
        for (int index = 0; index < groups.size(); ++index) {
            prefix = merge_property_group_segment(prefix, groups[index]);
            if (index == 0) {
                godot::ClassDB::add_property_group(p_class, groups[index].capitalize(), prefix + "_");
            } else {
                godot::ClassDB::add_property_subgroup(p_class, groups[index].capitalize(), prefix + "_");
            }
        }
    }
} // namespace libmaszyna::internal

#define INTERNAL_PROPERTY_GROUPS_FROM_PATH(p_groups)                                                                   \
    ::libmaszyna::internal::add_property_groups(get_class_static(), p_groups)

#define INTERNAL_BIND_PROPERTY_METHODS(p_class, p_name)                                                                \
    ClassDB::bind_method(                                                                                              \
            D_METHOD("set_" INTERNAL_PROPERTY_STRINGIFY(p_name), "value"),                                             \
            &p_class::INTERNAL_PROPERTY_SETTER(p_name));                                                               \
    ClassDB::bind_method(                                                                                              \
            D_METHOD("get_" INTERNAL_PROPERTY_STRINGIFY(p_name)), &p_class::INTERNAL_PROPERTY_GETTER(p_name));

#define INTERNAL_ADD_PROPERTY(p_property, p_name)                                                                      \
    ADD_PROPERTY(p_property, "set_" INTERNAL_PROPERTY_STRINGIFY(p_name), "get_" INTERNAL_PROPERTY_STRINGIFY(p_name));

#define INTERNAL_ADD_GROUPED_PROPERTY(p_property, p_name, p_groups)                                                    \
    INTERNAL_PROPERTY_GROUPS_FROM_PATH(p_groups);                                                                      \
    ADD_PROPERTY(p_property, "set_" INTERNAL_PROPERTY_STRINGIFY(p_name), "get_" INTERNAL_PROPERTY_STRINGIFY(p_name));

/**
 * Binds a property and its conventionally named setter and getter. An optional slash-separated grouping path accepts
 * a group and a subgroup. Group markers are generated from the path automatically.
 * <example>
 * BIND_PROPERTY(RailVehicleWheels, Variant::FLOAT, bogie_pivot_spacing);
 * BIND_PROPERTY(RailVehicleBrake, Variant::FLOAT, brake_force_max, "brake_force");
 * </example>
 */
#define INTERNAL_BIND_PROPERTY(p_class, p_type, p_name)                                                                \
    INTERNAL_BIND_PROPERTY_METHODS(p_class, p_name)                                                                    \
    INTERNAL_ADD_PROPERTY(PropertyInfo(p_type, INTERNAL_PROPERTY_STRINGIFY(p_name)), p_name)
#define INTERNAL_BIND_GROUPED_PROPERTY(p_class, p_type, p_name, p_groups)                                              \
    INTERNAL_BIND_PROPERTY_METHODS(p_class, p_name)                                                                    \
    INTERNAL_ADD_GROUPED_PROPERTY(PropertyInfo(p_type, INTERNAL_PROPERTY_STRINGIFY(p_name)), p_name, p_groups)
#define BIND_PROPERTY(...)                                                                                             \
    INTERNAL_PROPERTY_SELECT_4(__VA_ARGS__, INTERNAL_BIND_GROUPED_PROPERTY, INTERNAL_BIND_PROPERTY)(__VA_ARGS__)

/** Binds an Array property with PROPERTY_HINT_ARRAY_TYPE. */
#define INTERNAL_BIND_PROPERTY_ARRAY(p_class, p_name)                                                                  \
    INTERNAL_BIND_PROPERTY_METHODS(p_class, p_name)                                                                    \
    INTERNAL_ADD_PROPERTY(                                                                                             \
            PropertyInfo(Variant::ARRAY, INTERNAL_PROPERTY_STRINGIFY(p_name), PROPERTY_HINT_ARRAY_TYPE), p_name)
#define INTERNAL_BIND_GROUPED_PROPERTY_ARRAY(p_class, p_name, p_groups)                                                \
    INTERNAL_BIND_PROPERTY_METHODS(p_class, p_name)                                                                    \
    INTERNAL_ADD_GROUPED_PROPERTY(                                                                                     \
            PropertyInfo(Variant::ARRAY, INTERNAL_PROPERTY_STRINGIFY(p_name), PROPERTY_HINT_ARRAY_TYPE), p_name,       \
            p_groups)
#define BIND_PROPERTY_ARRAY(...)                                                                                       \
    INTERNAL_PROPERTY_SELECT_3(__VA_ARGS__, INTERNAL_BIND_GROUPED_PROPERTY_ARRAY, INTERNAL_BIND_PROPERTY_ARRAY)(       \
            __VA_ARGS__)

/** Binds a property with an Inspector hint. */
#define INTERNAL_BIND_PROPERTY_W_HINT(p_class, p_type, p_name, p_hint_type, p_hint_string)                             \
    INTERNAL_BIND_PROPERTY_METHODS(p_class, p_name)                                                                    \
    INTERNAL_ADD_PROPERTY(                                                                                             \
            PropertyInfo(p_type, INTERNAL_PROPERTY_STRINGIFY(p_name), p_hint_type, String(p_hint_string)), p_name)
#define INTERNAL_BIND_GROUPED_PROPERTY_W_HINT(p_class, p_type, p_name, p_groups, p_hint_type, p_hint_string)           \
    INTERNAL_BIND_PROPERTY_METHODS(p_class, p_name)                                                                    \
    INTERNAL_ADD_GROUPED_PROPERTY(                                                                                     \
            PropertyInfo(p_type, INTERNAL_PROPERTY_STRINGIFY(p_name), p_hint_type, String(p_hint_string)), p_name,     \
            p_groups)
#define BIND_PROPERTY_W_HINT(...)                                                                                      \
    INTERNAL_PROPERTY_SELECT_6(__VA_ARGS__, INTERNAL_BIND_GROUPED_PROPERTY_W_HINT, INTERNAL_BIND_PROPERTY_W_HINT)(     \
            __VA_ARGS__)

/** Binds a typed resource Array property. */
#define INTERNAL_BIND_PROPERTY_W_HINT_RES_ARRAY(p_class, p_type, p_name, p_hint_type, p_hint_resource_name)            \
    INTERNAL_BIND_PROPERTY_METHODS(p_class, p_name)                                                                    \
    INTERNAL_ADD_PROPERTY(                                                                                             \
            PropertyInfo(                                                                                              \
                    p_type, INTERNAL_PROPERTY_STRINGIFY(p_name), p_hint_type,                                          \
                    String::num(Variant::OBJECT) + "/" + String::num(PROPERTY_HINT_RESOURCE_TYPE) + ":" +              \
                            String(p_hint_resource_name),                                                              \
                    PROPERTY_USAGE_DEFAULT, "TypedArray<" + String(p_hint_resource_name) + ">"),                       \
            p_name)
#define INTERNAL_BIND_GROUPED_PROPERTY_W_HINT_RES_ARRAY(                                                               \
        p_class, p_type, p_name, p_groups, p_hint_type, p_hint_resource_name)                                          \
    INTERNAL_BIND_PROPERTY_METHODS(p_class, p_name)                                                                    \
    INTERNAL_ADD_GROUPED_PROPERTY(                                                                                     \
            PropertyInfo(                                                                                              \
                    p_type, INTERNAL_PROPERTY_STRINGIFY(p_name), p_hint_type,                                          \
                    String::num(Variant::OBJECT) + "/" + String::num(PROPERTY_HINT_RESOURCE_TYPE) + ":" +              \
                            String(p_hint_resource_name),                                                              \
                    PROPERTY_USAGE_DEFAULT, "TypedArray<" + String(p_hint_resource_name) + ">"),                       \
            p_name, p_groups)
#define BIND_PROPERTY_W_HINT_RES_ARRAY(...)                                                                            \
    INTERNAL_PROPERTY_SELECT_6(                                                                                        \
            __VA_ARGS__, INTERNAL_BIND_GROUPED_PROPERTY_W_HINT_RES_ARRAY,                                              \
            INTERNAL_BIND_PROPERTY_W_HINT_RES_ARRAY)(__VA_ARGS__)

#endif // MACROS_HPP
