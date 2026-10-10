#include "scenery/SceneryTrianglesSink.hpp"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <algorithm>
#include <array>

namespace godot {
    void SceneryTrianglesSink::_bind_methods() {
        ClassDB::bind_static_method(
                "SceneryTrianglesSink", D_METHOD("create", "directory", "budget_bytes"), &SceneryTrianglesSink::create,
                DEFVAL(BUDGET_BYTES));
        BIND_CONSTANT(BUDGET_BYTES);
        ClassDB::bind_static_method(
                "SceneryTrianglesSink", D_METHOD("cell_get_origin", "cell"), &SceneryTrianglesSink::cell_get_origin);
        ClassDB::bind_method(
                D_METHOD("add_triangles", "texture", "vertices", "normals", "uvs", "range_min", "range_max"),
                &SceneryTrianglesSink::add_triangles);
        ClassDB::bind_method(D_METHOD("add_geometry", "geometry"), &SceneryTrianglesSink::add_geometry);
        ClassDB::bind_method(D_METHOD("add_geometry_file", "path"), &SceneryTrianglesSink::add_geometry_file);
        ClassDB::bind_method(D_METHOD("get_geometries"), &SceneryTrianglesSink::get_geometries);
        ClassDB::bind_method(D_METHOD("finish"), &SceneryTrianglesSink::finish);
    }

    struct ClipVertex {
            Vector3 position;
            Vector3 normal;
            Vector2 uv;
    };

    // The cut is always computed from the lower end of the edge and lands exactly on the bound, so
    // two triangles sharing the edge get the very same vertex and no crack opens between them.
    static ClipVertex cut_edge(const ClipVertex &p_a, const ClipVertex &p_b, const int p_axis, const real_t p_bound) {
        const bool a_is_lower = p_a.position[p_axis] <= p_b.position[p_axis];
        const ClipVertex &from = a_is_lower ? p_a : p_b;
        const ClipVertex &to = a_is_lower ? p_b : p_a;
        const real_t weight = (p_bound - from.position[p_axis]) / (to.position[p_axis] - from.position[p_axis]);
        ClipVertex cut;
        cut.position = from.position.lerp(to.position, weight);
        cut.position[p_axis] = p_bound;
        cut.normal = from.normal.lerp(to.normal, weight).normalized();
        cut.uv = from.uv.lerp(to.uv, weight);
        return cut;
    }

    /// A triangle cut by the four sides of a cell: a convex polygon cut by one side gains at most
    /// one vertex, so three plus four
    constexpr int MAX_CLIP_VERTICES = 7;

    /// A clipped polygon on the stack - a heap allocation per triangle cost more than the cutting
    struct ClipPolygon {
            std::array<ClipVertex, MAX_CLIP_VERTICES> vertices;
            int count = 0;

            void push(const ClipVertex &p_vertex) {
                vertices[count++] = p_vertex;
            }
    };

    // Sutherland-Hodgman against one axis-aligned bound; the winding of the polygon is kept.
    static ClipPolygon
    clip_polygon(const ClipPolygon &p_polygon, const int p_axis, const real_t p_bound, const bool p_keep_above) {
        ClipPolygon clipped;
        for (int index = 0; index < p_polygon.count; index++) {
            const ClipVertex &current = p_polygon.vertices[index];
            const ClipVertex &next = p_polygon.vertices[(index + 1) % p_polygon.count];
            const bool current_inside =
                    p_keep_above ? current.position[p_axis] >= p_bound : current.position[p_axis] <= p_bound;
            const bool next_inside = p_keep_above ? next.position[p_axis] >= p_bound : next.position[p_axis] <= p_bound;
            if (current_inside) {
                clipped.push(current);
            }
            if (!(current_inside == next_inside)) {
                clipped.push(cut_edge(current, next, p_axis, p_bound));
            }
        }
        return clipped;
    }

    static ClipPolygon clip_to_cell_range(const ClipPolygon &p_polygon, const int p_axis, const int p_cell) {
        const real_t lower = static_cast<real_t>(static_cast<double>(p_cell) * SceneryTrianglesSink::CHUNK_SIZE_M);
        const real_t upper = static_cast<real_t>(static_cast<double>(p_cell + 1) * SceneryTrianglesSink::CHUNK_SIZE_M);
        return clip_polygon(clip_polygon(p_polygon, p_axis, lower, true), p_axis, upper, false);
    }

    /// The cell's vectors, relative to its centre, as floats
    struct Piece {
            Vector2i cell;
            std::vector<float> vertices;
            std::vector<float> normals;
            std::vector<float> uvs;
    };

    static void append_vertex(Piece &p_piece, const ClipVertex &p_vertex, const Vector3 &p_origin) {
        const Vector3 position = p_vertex.position - p_origin;
        p_piece.vertices.insert(
                p_piece.vertices.end(),
                {static_cast<float>(position.x), static_cast<float>(position.y), static_cast<float>(position.z)});
        p_piece.normals.insert(
                p_piece.normals.end(), {static_cast<float>(p_vertex.normal.x), static_cast<float>(p_vertex.normal.y),
                                        static_cast<float>(p_vertex.normal.z)});
        p_piece.uvs.insert(p_piece.uvs.end(), {static_cast<float>(p_vertex.uv.x), static_cast<float>(p_vertex.uv.y)});
    }

    static PackedFloat32Array to_packed(const std::vector<float> &p_values) {
        PackedFloat32Array packed;
        packed.resize(static_cast<int64_t>(p_values.size()));
        std::copy(p_values.begin(), p_values.end(), packed.ptrw());
        return packed;
    }

    static void append_packed(std::vector<float> &p_values, const PackedFloat32Array &p_packed) {
        p_values.reserve(p_values.size() + static_cast<size_t>(p_packed.size()));
        for (const float value: p_packed) {
            p_values.push_back(value);
        }
    }

    Ref<SceneryTrianglesSink> SceneryTrianglesSink::create(const String &p_directory, const int64_t p_budget_bytes) {
        Ref<SceneryTrianglesSink> sink;
        sink.instantiate();
        sink->directory = p_directory;
        sink->budget_bytes = p_budget_bytes;
        if (p_directory.is_empty()) {
            return sink;
        }
        DirAccess::make_dir_recursive_absolute(p_directory);
        for (const String &file: DirAccess::get_files_at(p_directory)) {
            DirAccess::remove_absolute(p_directory.path_join(file));
        }
        return sink;
    }

    Vector3 SceneryTrianglesSink::cell_get_origin(const Vector2i &p_cell) {
        return Vector3(
                static_cast<real_t>((static_cast<double>(p_cell.x) + 0.5) * CHUNK_SIZE_M), 0.0,
                static_cast<real_t>((static_cast<double>(p_cell.y) + 0.5) * CHUNK_SIZE_M));
    }

    String SceneryTrianglesSink::_get_chunk_key(
            const String &p_texture, const Vector2i &p_cell, const float p_range_min, const float p_range_max) {
        const String separator = "|";
        return p_texture + separator + String::num_int64(p_cell.x) + separator + String::num_int64(p_cell.y) +
               separator + String::num(p_range_min) + separator + String::num(p_range_max);
    }

    /// Under the mutex
    SceneryTrianglesSink::Chunk &SceneryTrianglesSink::_get_chunk(
            const String &p_texture, const Vector2i &p_cell, const float p_range_min, const float p_range_max) {
        const String key = _get_chunk_key(p_texture, p_cell, p_range_min, p_range_max);
        if (const int *found = chunk_indices.getptr(key); found != nullptr) {
            return chunks[*found];
        }
        chunk_indices[key] = static_cast<int>(chunks.size());
        Chunk &chunk = chunks.emplace_back();
        chunk.texture = p_texture;
        chunk.cell = p_cell;
        chunk.range_min = p_range_min;
        chunk.range_max = p_range_max;
        return chunk;
    }

    int64_t SceneryTrianglesSink::_get_bytes(const Chunk &p_chunk) {
        return static_cast<int64_t>(
                (p_chunk.vertices.size() + p_chunk.normals.size() + p_chunk.uvs.size()) * sizeof(float));
    }

    Ref<MaszynaTrianglesChunkGeometry> SceneryTrianglesSink::_to_geometry(const Chunk &p_chunk) {
        Ref<MaszynaTrianglesChunkGeometry> geometry;
        geometry.instantiate();
        geometry->set_texture(p_chunk.texture);
        geometry->set_cell(p_chunk.cell);
        geometry->set_range_min(p_chunk.range_min);
        geometry->set_range_max(p_chunk.range_max);
        geometry->set_vertices(to_packed(p_chunk.vertices));
        geometry->set_normals(to_packed(p_chunk.normals));
        geometry->set_uvs(to_packed(p_chunk.uvs));
        return geometry;
    }

    void SceneryTrianglesSink::_append(Chunk &p_chunk, const Ref<MaszynaTrianglesChunkGeometry> &p_geometry) {
        append_packed(p_chunk.vertices, p_geometry->get_vertices());
        append_packed(p_chunk.normals, p_geometry->get_normals());
        append_packed(p_chunk.uvs, p_geometry->get_uvs());
    }

    String SceneryTrianglesSink::_get_part_path(const int p_chunk, const int p_part) const {
        return directory.path_join(
                String::num_int64(p_chunk) + String("_") + String::num_int64(p_part) + String(".res"));
    }

    /// Under the mutex: the largest chunks go to disk until half the budget is left
    void SceneryTrianglesSink::_spill() {
        std::vector<int> order(chunks.size());
        for (size_t index = 0; index < order.size(); index++) {
            order[index] = static_cast<int>(index);
        }
        std::sort(order.begin(), order.end(), [this](const int p_left, const int p_right) {
            return _get_bytes(chunks[p_left]) > _get_bytes(chunks[p_right]);
        });
        for (const int index: order) {
            if (buffered_bytes <= budget_bytes / 2) {
                return;
            }
            Chunk &chunk = chunks[index];
            const Error error =
                    ResourceSaver::get_singleton()->save(_to_geometry(chunk), _get_part_path(index, chunk.parts));
            ERR_FAIL_COND_MSG(error != OK, "Cannot write a terrain chunk part to " + directory);
            chunk.parts++;
            buffered_bytes -= _get_bytes(chunk);
            std::vector<float>().swap(chunk.vertices);
            std::vector<float>().swap(chunk.normals);
            std::vector<float>().swap(chunk.uvs);
        }
    }

    /// A triangle is cut along the cell grid, each piece stored in its own cell: a chunk is streamed
    /// and culled by its cell (SceneryStreamingServer), so geometry reaching outside of it - terrain
    /// triangles can be kilometres long - went missing under the camera. Cut on the calling thread;
    /// only the appending holds the mutex.
    void SceneryTrianglesSink::add_triangles(
            const String &p_texture, const PackedVector3Array &p_vertices, const PackedVector3Array &p_normals,
            const PackedVector2Array &p_uvs, const float p_range_min, const float p_range_max) {
        std::vector<Piece> pieces;
        // a cell's piece is made by its first triangle with area: a cell the triangles only touch
        // gets none, and no empty chunk (FINDINGS.md, 10-02). Most nodes lie in one cell, so the
        // piece of the last cell is found again without a search
        Piece *last_piece = nullptr;
        const auto piece_of = [&](const Vector2i &p_cell) -> Piece & {
            if (last_piece != nullptr && last_piece->cell == p_cell) {
                return *last_piece;
            }
            auto found = std::find_if(
                    pieces.begin(), pieces.end(), [&p_cell](const Piece &p_piece) { return p_piece.cell == p_cell; });
            if (found == pieces.end()) {
                Piece &piece = pieces.emplace_back(Piece{p_cell, {}, {}, {}});
                if (pieces.size() == 1) {
                    piece.vertices.reserve(p_vertices.size() * MaszynaTrianglesChunkGeometry::VECTOR3_FLOATS);
                    piece.normals.reserve(p_vertices.size() * MaszynaTrianglesChunkGeometry::VECTOR3_FLOATS);
                    piece.uvs.reserve(p_vertices.size() * MaszynaTrianglesChunkGeometry::VECTOR2_FLOATS);
                }
                last_piece = &piece;
                return piece;
            }
            last_piece = &*found;
            return *found;
        };
        // a triangle touching the cell with an edge or a corner leaves no area in it
        const auto append_fan = [&](const Vector2i &p_cell, const ClipPolygon &p_polygon) {
            const Vector3 origin = cell_get_origin(p_cell);
            // the piece is convex, so a fan from its first vertex keeps the winding
            for (int corner = 1; corner + 1 < p_polygon.count; corner++) {
                const ClipVertex *fan[3] = {
                        p_polygon.vertices.data(), &p_polygon.vertices[corner], &p_polygon.vertices[corner + 1]};
                const Vector3 area = (fan[1]->position - fan[0]->position).cross(fan[2]->position - fan[0]->position);
                if (area.length_squared() < CMP_EPSILON2) {
                    continue;
                }
                Piece &piece = piece_of(p_cell);
                for (const ClipVertex *vertex: fan) {
                    append_vertex(piece, *vertex, origin);
                }
            }
        };
        for (int64_t base = 0; base + 2 < p_vertices.size(); base += 3) {
            ClipPolygon triangle;
            Vector3 lower = p_vertices[base];
            Vector3 upper = p_vertices[base];
            for (int vertex_offset = 0; vertex_offset < 3; vertex_offset++) {
                const int64_t index = base + vertex_offset;
                triangle.push({p_vertices[index], p_normals[index], p_uvs[index]});
                lower = lower.min(p_vertices[index]);
                upper = upper.max(p_vertices[index]);
            }
            const int first_x = static_cast<int>(Math::floor(lower.x / CHUNK_SIZE_M));
            const int last_x = static_cast<int>(Math::floor(upper.x / CHUNK_SIZE_M));
            const int first_z = static_cast<int>(Math::floor(lower.z / CHUNK_SIZE_M));
            const int last_z = static_cast<int>(Math::floor(upper.z / CHUNK_SIZE_M));
            if (first_x == last_x && first_z == last_z) {
                append_fan(Vector2i(first_x, first_z), triangle);
                continue;
            }
            for (int chunk_x = first_x; chunk_x <= last_x; chunk_x++) {
                const ClipPolygon strip = clip_to_cell_range(triangle, Vector3::AXIS_X, chunk_x);
                for (int chunk_z = first_z; chunk_z <= last_z && strip.count >= 3; chunk_z++) {
                    const ClipPolygon piece_polygon = clip_to_cell_range(strip, Vector3::AXIS_Z, chunk_z);
                    if (piece_polygon.count >= 3) {
                        append_fan(Vector2i(chunk_x, chunk_z), piece_polygon);
                    }
                }
            }
        }

        MutexLock lock(mutex);
        for (const Piece &piece: pieces) {
            Chunk &chunk = _get_chunk(p_texture, piece.cell, p_range_min, p_range_max);
            chunk.vertices.insert(chunk.vertices.end(), piece.vertices.begin(), piece.vertices.end());
            chunk.normals.insert(chunk.normals.end(), piece.normals.begin(), piece.normals.end());
            chunk.uvs.insert(chunk.uvs.end(), piece.uvs.begin(), piece.uvs.end());
            buffered_bytes += static_cast<int64_t>(
                    (piece.vertices.size() + piece.normals.size() + piece.uvs.size()) * sizeof(float));
        }
        if (!directory.is_empty() && buffered_bytes > budget_bytes) {
            _spill();
        }
    }

    void SceneryTrianglesSink::add_geometry(const Ref<MaszynaTrianglesChunkGeometry> &p_geometry) {
        ERR_FAIL_COND(p_geometry.is_null());
        MutexLock lock(mutex);
        Chunk &chunk = _get_chunk(
                p_geometry->get_texture(), p_geometry->get_cell(), p_geometry->get_range_min(),
                p_geometry->get_range_max());
        const int64_t bytes = _get_bytes(chunk);
        _append(chunk, p_geometry);
        buffered_bytes += _get_bytes(chunk) - bytes;
        if (!directory.is_empty() && buffered_bytes > budget_bytes) {
            _spill();
        }
    }

    void SceneryTrianglesSink::add_geometry_file(const String &p_path) {
        const Ref<MaszynaTrianglesChunkGeometry> geometry =
                ResourceLoader::get_singleton()->load(p_path, "", ResourceLoader::CACHE_MODE_IGNORE);
        ERR_FAIL_COND_MSG(geometry.is_null(), "Cannot read a terrain chunk: " + p_path);
        add_geometry(geometry);
    }

    TypedArray<MaszynaTrianglesChunkGeometry> SceneryTrianglesSink::get_geometries() const {
        MutexLock lock(mutex);
        TypedArray<MaszynaTrianglesChunkGeometry> geometries;
        for (const Chunk &chunk: chunks) {
            geometries.append(_to_geometry(chunk));
        }
        return geometries;
    }

    /// One chunk at a time: its parts read back, joined with what is still in memory, written whole
    Array SceneryTrianglesSink::finish() {
        MutexLock lock(mutex);
        ERR_FAIL_COND_V_MSG(directory.is_empty(), Array(), "A sink without a directory keeps its chunks in memory");
        Array descriptors;
        for (int index = 0; index < static_cast<int>(chunks.size()); index++) {
            Chunk &chunk = chunks[index];
            Chunk whole;
            whole.texture = chunk.texture;
            whole.cell = chunk.cell;
            whole.range_min = chunk.range_min;
            whole.range_max = chunk.range_max;
            for (int part = 0; part < chunk.parts; part++) {
                const String part_path = _get_part_path(index, part);
                const Ref<MaszynaTrianglesChunkGeometry> geometry =
                        ResourceLoader::get_singleton()->load(part_path, "", ResourceLoader::CACHE_MODE_IGNORE);
                if (geometry.is_valid()) {
                    _append(whole, geometry);
                }
                DirAccess::remove_absolute(part_path);
            }
            whole.vertices.insert(whole.vertices.end(), chunk.vertices.begin(), chunk.vertices.end());
            whole.normals.insert(whole.normals.end(), chunk.normals.begin(), chunk.normals.end());
            whole.uvs.insert(whole.uvs.end(), chunk.uvs.begin(), chunk.uvs.end());
            chunk = Chunk(); // its memory goes before the next chunk is read

            // a chunk is uploaded in one go when it is streamed: a large one goes as pieces
            const TypedArray<MaszynaTrianglesChunkGeometry> pieces = _to_geometry(whole)->split();
            for (int64_t piece = 0; piece < pieces.size(); piece++) {
                const String path = directory.path_join(
                        String::num_int64(index) + String("-") + String::num_int64(piece) + String(".res"));
                const Error error = ResourceSaver::get_singleton()->save(pieces[piece], path);
                if (error != OK) {
                    UtilityFunctions::push_error("[SceneryTrianglesSink] Cannot write a terrain chunk to " + path);
                    continue;
                }
                Dictionary descriptor;
                descriptor["path"] = path;
                descriptor["position"] = cell_get_origin(whole.cell);
                descriptor["texture"] = whole.texture;
                descriptor["range_min"] = whole.range_min;
                descriptor["range_max"] = whole.range_max;
                descriptors.append(descriptor);
            }
        }
        chunks.clear();
        chunk_indices.clear();
        buffered_bytes = 0;
        return descriptors;
    }
} // namespace godot
