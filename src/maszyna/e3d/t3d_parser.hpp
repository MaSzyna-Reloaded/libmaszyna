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
            /// The model to draw: its description with its geometry applied - its meshes in the GPU
            Ref<MaszynaE3DModel> parse(const Ref<FileAccess> &p_file) const;
            /// The model's description alone; a text model is read whole all the same
            Ref<MaszynaE3DModel> parse_description(const Ref<FileAccess> &p_file) const;
            /// The model's geometry alone (MaszynaE3DModelBuilder::build_geometry())
            Ref<MaszynaE3DModelGeometry> parse_geometry(const Ref<FileAccess> &p_file) const;
            /// The vertices of the submodel of that geometry index, in its own frame
            PackedVector3Array parse_submodel_positions(const Ref<FileAccess> &p_file, int p_index) const;

        protected:
            static void _bind_methods();

        private:
            using SubModelData = MaszynaE3DModelBuilder::SubModelData;

            /* The submodels in the order the builders take them: every one after the one linking it;
             * false for a model that cannot be read */
            bool _parse_submodels(const Ref<FileAccess> &p_file, std::vector<SubModelData> &p_r_submodels) const;
            static int _find_by_name(const std::vector<SubModelData> &p_submodels, int p_index, const String &p_name);
            static void _rotate_to_scenery_frame(std::vector<SubModelData> &p_submodels, int p_index);
    };
} // namespace godot
