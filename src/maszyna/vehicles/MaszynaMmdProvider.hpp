#pragma once
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {
    /* Where the vehicles' MMD files are read from, by their absolute paths: whether a file is there,
     * what it says and when it last changed. MaszynaVehicleMmdManager reads every MMD and every file
     * it includes through the one it holds (set_mmd_provider()) - MaszynaFileMmdProvider, the disk,
     * unless a tool that keeps a vehicle's MMD elsewhere - an editor writing it of a description of its
     * own - hands in one deriving from this, overriding _file_exists(), _read_file() and
     * _get_modified_time(). Called on the vehicles' worker threads. */
    class MaszynaMmdProvider : public RefCounted {
            GDCLASS(MaszynaMmdProvider, RefCounted)

        protected:
            static void _bind_methods();

            GDVIRTUAL1RC(bool, _file_exists, String)
            GDVIRTUAL1RC(PackedByteArray, _read_file, String)
            GDVIRTUAL1RC(uint64_t, _get_modified_time, String)

        public:
            virtual bool file_exists(const String &p_abs_path) const;
            /* What the file says; empty for none */
            virtual PackedByteArray read_file(const String &p_abs_path) const;
            /* When the file last changed, as FileAccess.get_modified_time() tells it; 0 for none */
            virtual uint64_t get_modified_time(const String &p_abs_path) const;
    };
} // namespace godot
