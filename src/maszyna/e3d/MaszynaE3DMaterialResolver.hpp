#pragma once
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/callable.hpp>

namespace godot {
    struct MaszynaE3DInstanceData;
    class MaszynaE3DSubModel;

    /// Calls the material resolver callable once per submodel, data path, skins, translucency and
    /// max texture size; later calls return the same material while it is alive (a scenery
    /// instances the same models thousands of times).
    class MaszynaE3DMaterialResolver {
        private:
            Callable callable;
            HashMap<String, ObjectID> materials; // not owned - a freed material is resolved again

        public:
            void set_callable(const Callable &p_callable);
            /* Every material is resolved again */
            void clear();
            /* p_translucency: MaszynaE3DInstanceTypes::Translucency */
            Ref<Material>
            resolve(const MaszynaE3DInstanceData &p_instance, MaszynaE3DSubModel *p_submodel, int p_translucency);
    };
} // namespace godot
