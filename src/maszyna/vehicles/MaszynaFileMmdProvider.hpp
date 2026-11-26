#pragma once
#include "MaszynaMmdProvider.hpp"

namespace godot {
    /* The vehicles' MMD files as they are on the disk: the provider MaszynaVehicleMmdManager holds
     * until another is handed in (MaszynaVehicleMmdManager.set_mmd_provider()) */
    class MaszynaFileMmdProvider : public MaszynaMmdProvider {
            GDCLASS(MaszynaFileMmdProvider, MaszynaMmdProvider)

        protected:
            static void _bind_methods();

        public:
            bool file_exists(const String &p_abs_path) const override;
            PackedByteArray read_file(const String &p_abs_path) const override;
            uint64_t get_modified_time(const String &p_abs_path) const override;
    };
} // namespace godot
