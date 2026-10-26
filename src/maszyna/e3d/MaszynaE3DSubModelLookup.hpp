#pragma once

#include "MaszynaE3DSubModel.hpp"
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot {
    /// The submodel of that name, in lower case, anywhere under p_submodels - E3D and T3D models nest
    /// their submodels, so a name is looked for in the whole tree, the first one found (TModel3d::GetFromName()); null
    /// for none
    inline MaszynaE3DSubModel *find_submodel(const TypedArray<MaszynaE3DSubModel> &p_submodels, const String &p_name) {
        for (int64_t index = 0; index < p_submodels.size(); index++) {
            const Ref<MaszynaE3DSubModel> submodel = p_submodels[index];
            if (submodel.is_null()) {
                continue;
            }
            if (submodel->get_name().to_lower() == p_name) {
                return submodel.ptr();
            }
            if (MaszynaE3DSubModel *found = find_submodel(submodel->get_submodels(), p_name); found != nullptr) {
                return found;
            }
        }
        return nullptr;
    }
} // namespace godot
