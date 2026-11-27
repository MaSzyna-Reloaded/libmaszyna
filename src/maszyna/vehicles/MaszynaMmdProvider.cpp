#include "MaszynaMmdProvider.hpp"

namespace godot {
    void MaszynaMmdProvider::_bind_methods() {
        ClassDB::bind_method(D_METHOD("file_exists", "abs_path"), &MaszynaMmdProvider::file_exists);
        ClassDB::bind_method(D_METHOD("read_file", "abs_path"), &MaszynaMmdProvider::read_file);
        ClassDB::bind_method(D_METHOD("get_modified_time", "abs_path"), &MaszynaMmdProvider::get_modified_time);
        GDVIRTUAL_BIND(_file_exists, "abs_path");
        GDVIRTUAL_BIND(_read_file, "abs_path");
        GDVIRTUAL_BIND(_get_modified_time, "abs_path");
    }

    bool MaszynaMmdProvider::file_exists(const String &p_abs_path) const {
        bool exists = false;
        GDVIRTUAL_CALL(_file_exists, p_abs_path, exists);
        return exists;
    }

    PackedByteArray MaszynaMmdProvider::read_file(const String &p_abs_path) const {
        PackedByteArray bytes;
        GDVIRTUAL_CALL(_read_file, p_abs_path, bytes);
        return bytes;
    }

    uint64_t MaszynaMmdProvider::get_modified_time(const String &p_abs_path) const {
        uint64_t time = 0;
        GDVIRTUAL_CALL(_get_modified_time, p_abs_path, time);
        return time;
    }
} // namespace godot
