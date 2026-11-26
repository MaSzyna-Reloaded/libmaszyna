#include "MaszynaFileMmdProvider.hpp"

#include <godot_cpp/classes/file_access.hpp>

namespace godot {
    void MaszynaFileMmdProvider::_bind_methods() {}

    bool MaszynaFileMmdProvider::file_exists(const String &p_abs_path) const {
        return FileAccess::file_exists(p_abs_path);
    }

    PackedByteArray MaszynaFileMmdProvider::read_file(const String &p_abs_path) const {
        return FileAccess::get_file_as_bytes(p_abs_path);
    }

    uint64_t MaszynaFileMmdProvider::get_modified_time(const String &p_abs_path) const {
        return FileAccess::get_modified_time(p_abs_path);
    }
} // namespace godot
