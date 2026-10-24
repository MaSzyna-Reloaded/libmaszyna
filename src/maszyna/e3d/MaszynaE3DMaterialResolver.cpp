#include "MaszynaE3DInstanceBackend.hpp"
#include "MaszynaE3DMaterialResolver.hpp"
#include <godot_cpp/core/object.hpp>

namespace godot {
    /// `p_callable(submodel, data_path, skins, translucency, max_texture_size) -> Material`,
    /// see MaszynaMaterialManager.get_submodel_material()
    void MaszynaE3DMaterialResolver::set_callable(const Callable &p_callable) {
        callable = p_callable;
        materials.clear();
    }

    void MaszynaE3DMaterialResolver::clear() {
        materials.clear();
    }

    Ref<Material> MaszynaE3DMaterialResolver::resolve(
            const MaszynaE3DInstanceData &p_instance, MaszynaE3DSubModel *p_submodel, const int p_translucency) {
        if (!callable.is_valid()) {
            return {};
        }
        const String key = String::num_uint64(static_cast<int64_t>(p_submodel->get_instance_id())) + "|" +
                           p_instance.data_path + "|" + String("|").join(p_instance.skins) + "|" +
                           String::num_int64(p_translucency) + "|" + String::num_int64(p_instance.max_texture_size);
        const HashMap<String, ObjectID>::ConstIterator cached = materials.find(key);
        if (cached != materials.end()) {
            Material *material = Object::cast_to<Material>(ObjectDB::get_instance(cached->value));
            if (material != nullptr) {
                return Ref(material);
            }
        }
        const Ref<Material> material = callable.call(
                Ref(p_submodel), p_instance.data_path, p_instance.skins, p_translucency, p_instance.max_texture_size);
        if (material.is_valid()) {
            materials[key] = material->get_instance_id();
        }
        return material;
    }
} // namespace godot
