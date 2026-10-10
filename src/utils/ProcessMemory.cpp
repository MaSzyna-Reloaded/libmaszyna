#include "utils/ProcessMemory.hpp"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

#ifdef __linux__
#include <unistd.h>
#endif
#ifdef __GLIBC__
#include <malloc.h>
#endif

namespace godot {
    void ProcessMemory::_bind_methods() {
        ClassDB::bind_static_method(
                "ProcessMemory", D_METHOD("get_resident_bytes"), &ProcessMemory::get_resident_bytes);
        ClassDB::bind_static_method("ProcessMemory", D_METHOD("release_unused"), &ProcessMemory::release_unused);
    }

    /// The second field of /proc/self/statm, in pages. Read through FileAccess, not a C++ stream:
    /// godot-cpp links libstdc++ statically (GODOTCPP_USE_STATIC_CPP), and in the release library
    /// a stream reading a number crashed in its locale (docs/findings-archive.md, 2026-10-02)
    int64_t ProcessMemory::get_resident_bytes() {
#ifdef __linux__
        const Ref<FileAccess> statm = FileAccess::open("/proc/self/statm", FileAccess::READ);
        if (statm.is_null()) {
            return -1;
        }
        const PackedStringArray fields = statm->get_line().split(" ", false);
        if (fields.size() < 2) {
            return -1;
        }
        return fields[1].to_int() * sysconf(_SC_PAGESIZE);
#else
        return -1;
#endif
    }

    void ProcessMemory::release_unused() {
#ifdef __GLIBC__
        malloc_trim(0);
#endif
    }
} // namespace godot
