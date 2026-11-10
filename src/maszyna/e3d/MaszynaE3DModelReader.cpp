#include "MaszynaE3DModelReader.hpp"
#include "maszyna/e3d/e3d_parser.hpp"
#include "maszyna/e3d/t3d_parser.hpp"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    namespace {
        /* What p_read gives of the file, by its extension's parser; empty when it cannot be opened */
        template<typename Result, typename Read>
        Result read_with_parser(const String &p_path, const Read &p_read) {
            const Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::ModeFlags::READ);
            if (file.is_null()) {
                UtilityFunctions::push_error("Failed to open model file: " + p_path);
                return {};
            }
            if (p_path.get_extension().to_lower() == "t3d") {
                MaszynaT3DParser *parser = memnew(MaszynaT3DParser);
                Result result = p_read(*parser, file);
                memdelete(parser);
                return result;
            }
            MaszynaE3DParser *parser = memnew(MaszynaE3DParser);
            Result result = p_read(*parser, file);
            memdelete(parser);
            return result;
        }
    } // namespace

    void MaszynaE3DModelReader::_bind_methods() {
        ClassDB::bind_static_method(
                "MaszynaE3DModelReader", D_METHOD("read_model", "path"), &MaszynaE3DModelReader::read_model);
        ClassDB::bind_static_method(
                "MaszynaE3DModelReader", D_METHOD("read_description", "path"),
                &MaszynaE3DModelReader::read_description);
        ClassDB::bind_static_method(
                "MaszynaE3DModelReader", D_METHOD("read_geometry", "path"), &MaszynaE3DModelReader::read_geometry);
        ClassDB::bind_static_method(
                "MaszynaE3DModelReader", D_METHOD("read_submodel_positions", "path", "index"),
                &MaszynaE3DModelReader::read_submodel_positions);
    }

    Ref<MaszynaE3DModel> MaszynaE3DModelReader::read_model(const String &p_path) {
        return read_with_parser<Ref<MaszynaE3DModel>>(
                p_path, [](const auto &p_parser, const Ref<FileAccess> &p_file) { return p_parser.parse(p_file); });
    }

    Ref<MaszynaE3DModel> MaszynaE3DModelReader::read_description(const String &p_path) {
        return read_with_parser<Ref<MaszynaE3DModel>>(p_path, [](const auto &p_parser, const Ref<FileAccess> &p_file) {
            return p_parser.parse_description(p_file);
        });
    }

    Ref<MaszynaE3DModelGeometry> MaszynaE3DModelReader::read_geometry(const String &p_path) {
        return read_with_parser<Ref<MaszynaE3DModelGeometry>>(
                p_path,
                [](const auto &p_parser, const Ref<FileAccess> &p_file) { return p_parser.parse_geometry(p_file); });
    }

    PackedVector3Array MaszynaE3DModelReader::read_submodel_positions(const String &p_path, const int p_index) {
        return read_with_parser<PackedVector3Array>(
                p_path, [p_index](const auto &p_parser, const Ref<FileAccess> &p_file) {
                    return p_parser.parse_submodel_positions(p_file, p_index);
                });
    }
} // namespace godot
