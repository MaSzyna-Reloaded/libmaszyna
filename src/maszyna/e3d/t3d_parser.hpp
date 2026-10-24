#pragma once
#include "maszyna/e3d/MaszynaE3DModel.hpp"
#include "maszyna/e3d/MaszynaE3DModelBuilder.hpp"
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/object.hpp>

#include <vector>

namespace godot {
    /// Reads the text model format (.t3d), the original's TModel3d::LoadFromTextFile()
    /// (Model3d.cpp:2412) and TSubModel::Load() (Model3d.cpp:239), into the same MaszynaE3DModel an .e3d
    /// gives
    class MaszynaT3DParser : public Object {
            GDCLASS(MaszynaT3DParser, Object)

        public:
            Ref<MaszynaE3DModel> parse(const Ref<FileAccess> &p_file) const;

        protected:
            static void _bind_methods();

        private:
            using SubModelData = MaszynaE3DModelBuilder::SubModelData;

            static int _find_by_name(const std::vector<SubModelData> &p_submodels, int p_index, const String &p_name);
            static void _rotate_to_scenery_frame(std::vector<SubModelData> &p_submodels, int p_index);
    };
} // namespace godot
