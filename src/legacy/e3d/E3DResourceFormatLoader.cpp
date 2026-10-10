#include "legacy/e3d/E3DResourceFormatLoader.hpp"
#include "legacy/e3d/e3d_parser.hpp"
#include "legacy/e3d/t3d_parser.hpp"
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

    void E3DResourceFormatLoader::_bind_methods() {}

    PackedStringArray E3DResourceFormatLoader::_get_recognized_extensions() const {
        return PackedStringArray({"e3d", "t3d"});
    }

    bool E3DResourceFormatLoader::_handles_type(const StringName &p_type) const {
        return p_type == StringName("E3DModel") || p_type == StringName("Resource");
    }

    String E3DResourceFormatLoader::_get_resource_type(const String &p_path) const {
        // a .t3d is read into the same model an .e3d gives
        if (const String extension = p_path.get_extension().to_lower(); extension == "e3d" || extension == "t3d") {
            return "E3DModel";
        }

        return "";
    }

    Variant E3DResourceFormatLoader::_load(
            const String &p_path, const String &p_original_path, bool p_use_sub_threads, int32_t p_cache_mode) const {

        Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::ModeFlags::READ);
        if (!file.is_valid()) {
            UtilityFunctions::push_error("Failed to open model file: " + p_path);
            return Variant();
        }

        if (p_path.get_extension().to_lower() == "t3d") {
            T3DParser *parser = memnew(T3DParser);
            Variant result = parser->parse(file);
            memdelete(parser);
            return result;
        }
        E3DParser *parser = memnew(E3DParser);
        Variant result = parser->parse(file);
        memdelete(parser);
        return result;
    }

} // namespace godot
