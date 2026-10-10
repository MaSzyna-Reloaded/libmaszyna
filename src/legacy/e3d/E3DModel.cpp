#include "E3DModel.hpp"

namespace godot {
    E3DModel::~E3DModel() {
        clear();
    }

    void E3DModel::clear() {
        for (int i = 0; i < submodels.size(); i++) {
            Ref<E3DSubModel> sm = submodels.get(i);
            if (sm.is_valid()) {
                sm->clear();
            }
        }
        submodels.clear();
        lights.clear();
        smoke_sources.clear();
    }

    void E3DModel::_bind_methods() {
        BIND_CONSTANT(FORMAT_VERSION);
        BIND_PROPERTY_W_HINT_RES_ARRAY(E3DModel, Variant::ARRAY, submodels, PROPERTY_HINT_ARRAY_TYPE, "E3DSubModel");
        BIND_PROPERTY(E3DModel, Variant::DICTIONARY, lights);
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                E3DModel, Variant::ARRAY, smoke_sources, PROPERTY_HINT_ARRAY_TYPE, "E3DModelSmokeSourceDefinition");
        ClassDB::bind_method(D_METHOD("register_light", "p_name", "p_entry"), &E3DModel::register_light);
        ClassDB::bind_method(D_METHOD("register_smoke_source", "p_entry"), &E3DModel::register_smoke_source);
        ClassDB::bind_method(D_METHOD("get_node", "p_path"), &E3DModel::get_node);
        ClassDB::bind_method(D_METHOD("get_node_or_null", "p_path"), &E3DModel::get_node_or_null);
    }

    TypedDictionary<String, E3DModelLightDefinition> E3DModel::get_lights() const {
        return lights;
    }

    void E3DModel::set_lights(const TypedDictionary<String, E3DModelLightDefinition> &p_lights) {
        lights = p_lights;
    }

    TypedArray<E3DModelSmokeSourceDefinition> E3DModel::get_smoke_sources() const {
        return smoke_sources;
    }

    void E3DModel::set_smoke_sources(const TypedArray<E3DModelSmokeSourceDefinition> &p_smoke_sources) {
        smoke_sources = p_smoke_sources;
    }

    TypedArray<E3DSubModel> E3DModel::get_submodels() const {
        return submodels;
    }

    void E3DModel::set_submodels(const TypedArray<E3DSubModel> &p_submodels) {
        submodels = p_submodels;
    }

    void E3DModel::register_light(const String &p_name, const Ref<E3DModelLightDefinition> &p_entry) {
        lights[p_name] = p_entry;
    }

    void E3DModel::register_smoke_source(const Ref<E3DModelSmokeSourceDefinition> &p_entry) {
        smoke_sources.append(p_entry);
    }

    void E3DModel::add_child(const Ref<E3DSubModel> &p_sub_model) {
        submodels.append(p_sub_model);
    }

    PackedStringArray E3DModel::_split_path(const NodePath &p_path) const {
        PackedStringArray result;
        if (p_path.is_empty()) {
            return result;
        }

        const String raw_path = String(p_path);
        if (raw_path.is_empty()) {
            return result;
        }

        const PackedStringArray parts = raw_path.split("/");
        for (int i = 0; i < parts.size(); i++) {
            const String &segment = parts[i];
            if (segment.is_empty()) {
                result.clear();
                return result;
            }
            if (segment == "." || segment == "..") {
                result.clear();
                return result;
            }
            result.append(segment);
        }

        return result;
    }

    Ref<E3DSubModel> E3DModel::_get_node_or_null(const PackedStringArray &p_segments) const {
        if (p_segments.is_empty()) {
            return nullptr;
        }

        TypedArray<E3DSubModel> current_level = submodels;
        Ref<E3DSubModel> current;

        for (int segment_index = 0; segment_index < p_segments.size(); segment_index++) {
            const String &segment = p_segments[segment_index];
            Ref<E3DSubModel> match;
            bool duplicate_found = false;

            for (int child_index = 0; child_index < current_level.size(); child_index++) {
                const Ref<E3DSubModel> child = current_level[child_index];
                if (!child.is_valid()) {
                    continue;
                }
                if (!(segment == child->get_name())) {
                    continue;
                }

                if (match.is_valid()) {
                    duplicate_found = true;
                    break;
                }

                match = child;
            }

            if (duplicate_found) {
                return nullptr;
            }
            if (!match.is_valid()) {
                return nullptr;
            }

            current = match;
            current_level = current->get_submodels();
        }

        return current;
    }

    Ref<E3DSubModel> E3DModel::get_node_or_null(const NodePath &p_path) const {
        const PackedStringArray segments = _split_path(p_path);
        if (segments.is_empty()) {
            return nullptr;
        }

        return _get_node_or_null(segments);
    }

    Ref<E3DSubModel> E3DModel::get_node(const NodePath &p_path) const {
        const PackedStringArray segments = _split_path(p_path);
        if (segments.is_empty()) {
            UtilityFunctions::push_error("[E3DModel] Invalid NodePath");
            return nullptr;
        }

        const Ref<E3DSubModel> node = _get_node_or_null(segments);
        if (!node.is_valid()) {
            UtilityFunctions::push_error("[E3DModel] Node not found or path is ambiguous: " + String(p_path));
        }

        return node;
    }
} // namespace godot
