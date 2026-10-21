#include "MaszynaE3DModel.hpp"
#include "macros.hpp"

namespace godot {
    MaszynaE3DModel::~MaszynaE3DModel() {
        clear();
    }

    void MaszynaE3DModel::clear() {
        for (int i = 0; i < submodels.size(); i++) {
            Ref<MaszynaE3DSubModel> sm = submodels.get(i);
            if (sm.is_valid()) {
                sm->clear();
            }
        }
        submodels.clear();
        lights.clear();
        smoke_sources.clear();
    }

    void MaszynaE3DModel::_bind_methods() {
        BIND_CONSTANT(FORMAT_VERSION);
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                MaszynaE3DModel, Variant::ARRAY, submodels, PROPERTY_HINT_ARRAY_TYPE, "MaszynaE3DSubModel");
        BIND_PROPERTY(MaszynaE3DModel, Variant::DICTIONARY, lights);
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                MaszynaE3DModel, Variant::ARRAY, smoke_sources, PROPERTY_HINT_ARRAY_TYPE,
                "MaszynaE3DModelSmokeSourceDefinition");
        ClassDB::bind_method(D_METHOD("register_light", "p_name", "p_entry"), &MaszynaE3DModel::register_light);
        ClassDB::bind_method(D_METHOD("register_smoke_source", "p_entry"), &MaszynaE3DModel::register_smoke_source);
        ClassDB::bind_method(D_METHOD("get_node", "p_path"), &MaszynaE3DModel::get_node);
        ClassDB::bind_method(D_METHOD("get_node_or_null", "p_path"), &MaszynaE3DModel::get_node_or_null);
    }

    TypedDictionary<String, MaszynaE3DModelLightDefinition> MaszynaE3DModel::get_lights() const {
        return lights;
    }

    void MaszynaE3DModel::set_lights(const TypedDictionary<String, MaszynaE3DModelLightDefinition> &p_lights) {
        lights = p_lights;
    }

    TypedArray<MaszynaE3DModelSmokeSourceDefinition> MaszynaE3DModel::get_smoke_sources() const {
        return smoke_sources;
    }

    void MaszynaE3DModel::set_smoke_sources(const TypedArray<MaszynaE3DModelSmokeSourceDefinition> &p_smoke_sources) {
        smoke_sources = p_smoke_sources;
    }

    TypedArray<MaszynaE3DSubModel> MaszynaE3DModel::get_submodels() const {
        return submodels;
    }

    void MaszynaE3DModel::set_submodels(const TypedArray<MaszynaE3DSubModel> &p_submodels) {
        submodels = p_submodels;
    }

    void MaszynaE3DModel::register_light(const String &p_name, const Ref<MaszynaE3DModelLightDefinition> &p_entry) {
        lights[p_name] = p_entry;
    }

    void MaszynaE3DModel::register_smoke_source(const Ref<MaszynaE3DModelSmokeSourceDefinition> &p_entry) {
        smoke_sources.append(p_entry);
    }

    void MaszynaE3DModel::add_child(const Ref<MaszynaE3DSubModel> &p_sub_model) {
        submodels.append(p_sub_model);
    }

    PackedStringArray MaszynaE3DModel::_split_path(const NodePath &p_path) const {
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

    Ref<MaszynaE3DSubModel> MaszynaE3DModel::_get_node_or_null(const PackedStringArray &p_segments) const {
        if (p_segments.is_empty()) {
            return nullptr;
        }

        TypedArray<MaszynaE3DSubModel> current_level = submodels;
        Ref<MaszynaE3DSubModel> current;

        for (int segment_index = 0; segment_index < p_segments.size(); segment_index++) {
            const String &segment = p_segments[segment_index];
            Ref<MaszynaE3DSubModel> match;
            bool duplicate_found = false;

            for (int child_index = 0; child_index < current_level.size(); child_index++) {
                const Ref<MaszynaE3DSubModel> child = current_level[child_index];
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

    Ref<MaszynaE3DSubModel> MaszynaE3DModel::get_node_or_null(const NodePath &p_path) const {
        const PackedStringArray segments = _split_path(p_path);
        if (segments.is_empty()) {
            return nullptr;
        }

        return _get_node_or_null(segments);
    }

    Ref<MaszynaE3DSubModel> MaszynaE3DModel::get_node(const NodePath &p_path) const {
        const PackedStringArray segments = _split_path(p_path);
        if (segments.is_empty()) {
            UtilityFunctions::push_error("[MaszynaE3DModel] Invalid NodePath");
            return nullptr;
        }

        const Ref<MaszynaE3DSubModel> node = _get_node_or_null(segments);
        if (!node.is_valid()) {
            UtilityFunctions::push_error("[MaszynaE3DModel] Node not found or path is ambiguous: " + String(p_path));
        }

        return node;
    }
} // namespace godot
