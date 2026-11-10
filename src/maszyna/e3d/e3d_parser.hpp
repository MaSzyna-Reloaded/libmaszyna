#pragma once
#include "maszyna/e3d/MaszynaE3DModel.hpp"
#include "maszyna/e3d/MaszynaE3DModelBuilder.hpp"
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/object.hpp>

#include <vector>

namespace godot {
    class MaszynaE3DParser : public Object {
            GDCLASS(MaszynaE3DParser, Object)

        public:
            /// The model to draw: its description with its geometry applied - its meshes in the GPU
            Ref<MaszynaE3DModel> parse(const Ref<FileAccess> &p_file) const;
            /// The model's description alone: the vertex and index chunks are skipped, nothing of
            /// them is read
            Ref<MaszynaE3DModel> parse_description(const Ref<FileAccess> &p_file) const;
            /// The model's geometry alone (MaszynaE3DModelBuilder::build_geometry())
            Ref<MaszynaE3DModelGeometry> parse_geometry(const Ref<FileAccess> &p_file) const;
            /// The vertices of the submodel of that geometry index, in its own frame - only they are
            /// read of the vertex chunk
            PackedVector3Array parse_submodel_positions(const Ref<FileAccess> &p_file, int p_index) const;

        protected:
            static void _bind_methods();

        private:
            const int64_t max_31_b = 1LL << 31;
            const int64_t max_32_b = 1LL << 32;
            /// Bytes per vertex: position, normal, uv (VNT0), plus a tangent (VNT2) - Model3d.cpp:1984
            static constexpr uint64_t VNT0_VERTEX_SIZE = 32;
            static constexpr uint64_t VNT2_VERTEX_SIZE = 48;
            /// iFlags bit of a translucent submodel (Model3d.cpp:455)
            static constexpr uint32_t SUBMODEL_FLAG_TRANSLUCENT = 0x20;
            /// Bytes of one submodel record: 256 + 64 per chunk version (Model3d.cpp:1948)
            static constexpr int SUB0_SUBMODEL_SIZE = 256;
            static constexpr int SUB1_SUBMODEL_SIZE = 320;
            /// Bytes of a submodel record read field by field; the rest is skipped
            /// (TSubModel::deserialize, Model3d.cpp:1873-1916)
            static constexpr int SUBMODEL_READ_SIZE = 168;
            /// Bytes of one TRA0 matrix, 16 floats (Model3d.cpp:2129)
            static constexpr int TRA0_MATRIX_SIZE = 64;
            using SubModelData = MaszynaE3DModelBuilder::SubModelData;

            struct ChunkHeader {
                    String id;
                    uint32_t len;
                    uint32_t data_len;
            };

            ChunkHeader _read_chunk_header(const Ref<FileAccess> &p_file) const;
            int u32s(uint32_t p_value) const;
            SubModelData _read_submodel(const Ref<FileAccess> &p_file, int p_chunk_size) const;
            /* Where the vertex chunk is and how long a vertex in it is (VNT0, VNT2), 0 without one */
            struct VertexChunk {
                    uint64_t position = 0;
                    uint64_t vertex_size = 0;
            };

            /* The submodels; with p_geometry their arrays too, without it the vertex and index chunks
             * are only located (r_vertices) */
            std::vector<SubModelData>
            _parse_file(const Ref<FileAccess> &p_file, bool p_geometry, VertexChunk &r_vertices) const;
            std::vector<String> _buffer_to_strings(const PackedByteArray &p_buffer) const;
    };
} // namespace godot
