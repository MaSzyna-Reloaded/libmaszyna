#include "maszyna/scenery/MaszynaTrianglesSink.hpp"

#include "cache/ResourceCache.hpp"
#include "maszyna/scenery/MaszynaTrianglesSector.hpp"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <algorithm>
#include <array>

namespace godot {
    void MaszynaTrianglesSink::_bind_methods() {
        ClassDB::bind_static_method(
                "MaszynaTrianglesSink", D_METHOD("create", "directory", "translucency", "budget_bytes"),
                &MaszynaTrianglesSink::create, DEFVAL(BUDGET_BYTES));
        BIND_CONSTANT(BUDGET_BYTES);
        ClassDB::bind_static_method(
                "MaszynaTrianglesSink", D_METHOD("cell_get_origin", "cell"), &MaszynaTrianglesSink::cell_get_origin);
        ClassDB::bind_static_method(
                "MaszynaTrianglesSink", D_METHOD("shape_is_ranged", "translucent", "range_min", "range_max"),
                &MaszynaTrianglesSink::shape_is_ranged);
        ClassDB::bind_method(
                D_METHOD("add_triangles", "texture", "vertices", "normals", "uvs", "range_min", "range_max"),
                &MaszynaTrianglesSink::add_triangles);
        ClassDB::bind_method(D_METHOD("add_geometry", "geometry"), &MaszynaTrianglesSink::add_geometry);
        ClassDB::bind_method(D_METHOD("add_geometry_file", "path"), &MaszynaTrianglesSink::add_geometry_file);
        ClassDB::bind_method(D_METHOD("get_geometries"), &MaszynaTrianglesSink::get_geometries);
        ClassDB::bind_method(D_METHOD("finish"), &MaszynaTrianglesSink::finish);
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
        const real_t lower = static_cast<real_t>(static_cast<double>(p_cell) * MaszynaTrianglesSink::CHUNK_SIZE_M);
        const real_t upper = static_cast<real_t>(static_cast<double>(p_cell + 1) * MaszynaTrianglesSink::CHUNK_SIZE_M);
        return clip_polygon(clip_polygon(p_polygon, p_axis, lower, true), p_axis, upper, false);
    }

    /// A section's or a shape's vectors, relative to its origin, as floats
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

    Ref<MaszynaTrianglesSink> MaszynaTrianglesSink::create(
            const String &p_directory, const Callable &p_translucency, const int64_t p_budget_bytes) {
        Ref<MaszynaTrianglesSink> sink;
        sink.instantiate();
        sink->directory = p_directory;
        sink->translucency = p_translucency;
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

    Vector3 MaszynaTrianglesSink::cell_get_origin(const Vector2i &p_cell) {
        return Vector3(
                static_cast<real_t>((static_cast<double>(p_cell.x) + 0.5) * CHUNK_SIZE_M), 0.0,
                static_cast<real_t>((static_cast<double>(p_cell.y) + 0.5) * CHUNK_SIZE_M));
    }

    bool
    MaszynaTrianglesSink::shape_is_ranged(const bool p_translucent, const float p_range_min, const float p_range_max) {
        return p_translucent || (p_range_max > 0.0 && p_range_max <= CELL_RANGE_MAX_M) || p_range_min > 0.0;
    }

    /// Asked once per texture; the call is made outside the mutex, two workers may make it at once
    bool MaszynaTrianglesSink::_is_translucent(const String &p_texture) {
        {
            MutexLock lock(mutex);
            if (const bool *known = translucent_textures.getptr(p_texture); known != nullptr) {
                return *known;
            }
        }
        const bool translucent = static_cast<bool>(translucency.call(p_texture));
        MutexLock lock(mutex);
        translucent_textures[p_texture] = translucent;
        return translucent;
    }

    /// Under the mutex
    MaszynaTrianglesSink::Chunk &MaszynaTrianglesSink::_get_chunk(
            const String &p_key, const String &p_texture, const float p_range_min, const float p_range_max,
            const Vector3 &p_origin) {
        if (const int *found = chunk_indices.getptr(p_key); found != nullptr) {
            return chunks[*found];
        }
        chunk_indices[p_key] = static_cast<int>(chunks.size());
        Chunk &chunk = chunks.emplace_back();
        chunk.texture = p_texture;
        chunk.range_min = p_range_min;
        chunk.range_max = p_range_max;
        chunk.origin = p_origin;
        return chunk;
    }

    /// Under the mutex. A section's chunk is one group, centred on the section, without a range: the
    /// original draws a section's shapes whatever their range (Render(shape, true),
    /// opengl33renderer.cpp:2495)
    MaszynaTrianglesSink::Chunk &
    MaszynaTrianglesSink::_get_section_chunk(const String &p_texture, const Vector2i &p_section) {
        const String separator = "|";
        Chunk &chunk = _get_chunk(
                String("section") + separator + p_texture + separator + String::num_int64(p_section.x) + separator +
                        String::num_int64(p_section.y),
                p_texture, 0.0, 0.0, cell_get_origin(p_section));
        if (chunk.groups.empty()) {
            chunk.groups.emplace_back().centre = chunk.origin;
        }
        return chunk;
    }

    /// Under the mutex: the chunk of the CELL_SIZE_M cell the centre is in (basic_section::cell(),
    /// scene.cpp:849-852)
    MaszynaTrianglesSink::Chunk &MaszynaTrianglesSink::_get_cell_chunk(
            const String &p_texture, const float p_range_min, const float p_range_max, const Vector3 &p_centre) {
        const Vector2i cell(
                static_cast<int>(Math::floor(p_centre.x / CELL_SIZE_M)),
                static_cast<int>(Math::floor(p_centre.z / CELL_SIZE_M)));
        const String separator = "|";
        const Vector3 origin(
                static_cast<real_t>((static_cast<double>(cell.x) + 0.5) * CELL_SIZE_M), 0.0,
                static_cast<real_t>((static_cast<double>(cell.y) + 0.5) * CELL_SIZE_M));
        return _get_chunk(
                String("cell") + separator + p_texture + separator + String::num_int64(cell.x) + separator +
                        String::num_int64(cell.y) + separator + String::num(p_range_min) + separator +
                        String::num(p_range_max),
                p_texture, p_range_min, p_range_max, origin);
    }

    /// basic_cell::insert() (scene.cpp:279-296): the first group of the cell's chunk - the same
    /// texture and range - whose centre is closer than MERGE_DISTANCE_M takes the shape, and its
    /// centre moves towards the shape's by the shape's share of the vertices (shape_node::merge(),
    /// scenenode.cpp:436-439); otherwise the shape starts a group of its own
    MaszynaTrianglesSink::Group &
    MaszynaTrianglesSink::_get_group(Chunk &p_chunk, const Vector3 &p_centre, const int64_t p_vertex_count) {
        for (Group &group: p_chunk.groups) {
            if (group.centre.distance_squared_to(p_centre) < MERGE_DISTANCE_M * MERGE_DISTANCE_M) {
                group.centre = group.centre.lerp(
                        p_centre,
                        static_cast<real_t>(p_vertex_count) / static_cast<real_t>(p_vertex_count + group.vertex_count));
                group.vertex_count += p_vertex_count;
                return group;
            }
        }
        Group &group = p_chunk.groups.emplace_back();
        group.centre = p_centre;
        group.vertex_count = p_vertex_count;
        return group;
    }

    int64_t MaszynaTrianglesSink::_get_bytes(const Group &p_group) {
        return static_cast<int64_t>(
                (p_group.vertices.size() + p_group.normals.size() + p_group.uvs.size()) * sizeof(float));
    }

    /// The groups one after another, their centres relative to the chunk's origin
    Ref<MaszynaTrianglesChunkGeometry> MaszynaTrianglesSink::_to_geometry(const Chunk &p_chunk) {
        std::vector<float> vertices;
        std::vector<float> normals;
        std::vector<float> uvs;
        PackedVector3Array centres;
        PackedInt32Array ends;
        for (const Group &group: p_chunk.groups) {
            vertices.insert(vertices.end(), group.vertices.begin(), group.vertices.end());
            normals.insert(normals.end(), group.normals.begin(), group.normals.end());
            uvs.insert(uvs.end(), group.uvs.begin(), group.uvs.end());
            centres.push_back(group.centre - p_chunk.origin);
            ends.push_back(static_cast<int32_t>(vertices.size() / MaszynaTrianglesChunkGeometry::VECTOR3_FLOATS));
        }
        Ref<MaszynaTrianglesChunkGeometry> geometry;
        geometry.instantiate();
        geometry->set_texture(p_chunk.texture);
        geometry->set_origin(p_chunk.origin);
        geometry->set_range_min(p_chunk.range_min);
        geometry->set_range_max(p_chunk.range_max);
        geometry->set_vertices(to_packed(vertices));
        geometry->set_normals(to_packed(normals));
        geometry->set_uvs(to_packed(uvs));
        geometry->set_group_centres(centres);
        geometry->set_group_ends(ends);
        return geometry;
    }

    /// A part written by _spill(): its groups are the chunk's first ones, by their index
    void MaszynaTrianglesSink::_append_part(Chunk &p_chunk, const Ref<MaszynaTrianglesChunkGeometry> &p_part) {
        const PackedFloat32Array vertices = p_part->get_vertices();
        const PackedFloat32Array normals = p_part->get_normals();
        const PackedFloat32Array uvs = p_part->get_uvs();
        const PackedInt32Array ends = p_part->get_group_ends();
        int64_t first = 0;
        for (int64_t index = 0; index < ends.size() && index < static_cast<int64_t>(p_chunk.groups.size()); index++) {
            Group &group = p_chunk.groups[index];
            const int64_t last = ends[index];
            for (int64_t vertex = first; vertex < last; vertex++) {
                for (int axis = 0; axis < MaszynaTrianglesChunkGeometry::VECTOR3_FLOATS; axis++) {
                    group.vertices.push_back(vertices[(vertex * MaszynaTrianglesChunkGeometry::VECTOR3_FLOATS) + axis]);
                    group.normals.push_back(normals[(vertex * MaszynaTrianglesChunkGeometry::VECTOR3_FLOATS) + axis]);
                }
                for (int axis = 0; axis < MaszynaTrianglesChunkGeometry::VECTOR2_FLOATS; axis++) {
                    group.uvs.push_back(uvs[(vertex * MaszynaTrianglesChunkGeometry::VECTOR2_FLOATS) + axis]);
                }
            }
            first = last;
        }
    }

    String MaszynaTrianglesSink::_get_part_path(const int p_chunk, const int p_part) const {
        return directory.path_join(
                String::num_int64(p_chunk) + String("_") + String::num_int64(p_part) + String(".res"));
    }

    /// Under the mutex: the largest chunks go to disk until half the budget is left; their groups
    /// keep their centres and counts
    void MaszynaTrianglesSink::_spill() {
        std::vector<int64_t> sizes(chunks.size(), 0);
        std::vector<int> order(chunks.size());
        for (size_t index = 0; index < order.size(); index++) {
            order[index] = static_cast<int>(index);
            for (const Group &group: chunks[index].groups) {
                sizes[index] += _get_bytes(group);
            }
        }
        std::sort(order.begin(), order.end(), [&sizes](const int p_left, const int p_right) {
            return sizes[p_left] > sizes[p_right];
        });
        for (const int index: order) {
            if (buffered_bytes <= budget_bytes / 2) {
                return;
            }
            Chunk &chunk = chunks[index];
            const Error error = ResourceCache::resource_save(_to_geometry(chunk), _get_part_path(index, chunk.parts));
            ERR_FAIL_COND_MSG(error != OK, "Cannot write a terrain chunk part to " + directory);
            chunk.parts++;
            buffered_bytes -= sizes[index];
            for (Group &group: chunk.groups) {
                std::vector<float>().swap(group.vertices);
                std::vector<float>().swap(group.normals);
                std::vector<float>().swap(group.uvs);
            }
        }
    }

    /// A ranged shape is kept whole, as the original keeps it, in the group around its centre - the
    /// mean of its vertices (shape_node::compute_radius(), scenenode.cpp:405-411).
    ///
    /// A section's triangle is cut along the 1 km grid, each piece stored in its own section: a chunk
    /// is streamed and culled by its section (SceneryStreamingServer), so geometry reaching outside of
    /// it - terrain triangles can be kilometres long - went missing under the camera. Cut on the
    /// calling thread; only the appending holds the mutex.
    void MaszynaTrianglesSink::add_triangles(
            const String &p_texture, const PackedVector3Array &p_vertices, const PackedVector3Array &p_normals,
            const PackedVector2Array &p_uvs, const float p_range_min, const float p_range_max) {
        if (p_vertices.is_empty()) {
            return;
        }
        if (shape_is_ranged(_is_translucent(p_texture), p_range_min, p_range_max)) {
            Vector3 centre;
            for (const Vector3 &vertex: p_vertices) {
                centre += vertex;
            }
            centre /= static_cast<real_t>(p_vertices.size());
            MutexLock lock(mutex);
            Chunk &chunk = _get_cell_chunk(p_texture, p_range_min, p_range_max, centre);
            Group &group = _get_group(chunk, centre, p_vertices.size());
            Piece shape{Vector2i(), {}, {}, {}};
            for (int64_t index = 0; index < p_vertices.size(); index++) {
                append_vertex(shape, {p_vertices[index], p_normals[index], p_uvs[index]}, chunk.origin);
            }
            group.vertices.insert(group.vertices.end(), shape.vertices.begin(), shape.vertices.end());
            group.normals.insert(group.normals.end(), shape.normals.begin(), shape.normals.end());
            group.uvs.insert(group.uvs.end(), shape.uvs.begin(), shape.uvs.end());
            buffered_bytes += static_cast<int64_t>(
                    (shape.vertices.size() + shape.normals.size() + shape.uvs.size()) * sizeof(float));
            if (!directory.is_empty() && buffered_bytes > budget_bytes) {
                _spill();
            }
            return;
        }
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
            Group &group = _get_section_chunk(p_texture, piece.cell).groups[0];
            group.vertices.insert(group.vertices.end(), piece.vertices.begin(), piece.vertices.end());
            group.normals.insert(group.normals.end(), piece.normals.begin(), piece.normals.end());
            group.uvs.insert(group.uvs.end(), piece.uvs.begin(), piece.uvs.end());
            buffered_bytes += static_cast<int64_t>(
                    (piece.vertices.size() + piece.normals.size() + piece.uvs.size()) * sizeof(float));
        }
        if (!directory.is_empty() && buffered_bytes > budget_bytes) {
            _spill();
        }
    }

    /// Each group of the geometry is a shape: a section's goes to the section of its origin, a
    /// ranged one to the group of its centre
    void MaszynaTrianglesSink::add_geometry(const Ref<MaszynaTrianglesChunkGeometry> &p_geometry) {
        ERR_FAIL_COND(p_geometry.is_null());
        const String texture = p_geometry->get_texture();
        const float range_min = p_geometry->get_range_min();
        const float range_max = p_geometry->get_range_max();
        const bool ranged = shape_is_ranged(_is_translucent(texture), range_min, range_max);
        const Vector3 origin = p_geometry->get_origin();
        const PackedFloat32Array vertices = p_geometry->get_vertices();
        const PackedFloat32Array normals = p_geometry->get_normals();
        const PackedFloat32Array uvs = p_geometry->get_uvs();
        const PackedVector3Array centres = p_geometry->get_group_centres();
        const PackedInt32Array ends = p_geometry->get_group_ends();
        MutexLock lock(mutex);
        int64_t first = 0;
        for (int64_t index = 0; index < ends.size(); index++) {
            const int64_t last = ends[index];
            const Vector3 centre = origin + centres[index];
            Chunk &chunk = ranged ? _get_cell_chunk(texture, range_min, range_max, centre)
                                  : _get_section_chunk(
                                            texture, Vector2i(
                                                             static_cast<int>(Math::floor(origin.x / CHUNK_SIZE_M)),
                                                             static_cast<int>(Math::floor(origin.z / CHUNK_SIZE_M))));
            Group &group = ranged ? _get_group(chunk, centre, last - first) : chunk.groups[0];
            const Vector3 offset = origin - chunk.origin;
            for (int64_t vertex = first; vertex < last; vertex++) {
                const int64_t vector3 = vertex * MaszynaTrianglesChunkGeometry::VECTOR3_FLOATS;
                const int64_t vector2 = vertex * MaszynaTrianglesChunkGeometry::VECTOR2_FLOATS;
                group.vertices.insert(
                        group.vertices.end(), {vertices[vector3] + static_cast<float>(offset.x),
                                               vertices[vector3 + 1] + static_cast<float>(offset.y),
                                               vertices[vector3 + 2] + static_cast<float>(offset.z)});
                group.normals.insert(
                        group.normals.end(), {normals[vector3], normals[vector3 + 1], normals[vector3 + 2]});
                group.uvs.insert(group.uvs.end(), {uvs[vector2], uvs[vector2 + 1]});
            }
            buffered_bytes += static_cast<int64_t>(
                    (last - first) *
                    ((2 * MaszynaTrianglesChunkGeometry::VECTOR3_FLOATS) +
                     MaszynaTrianglesChunkGeometry::VECTOR2_FLOATS) *
                    sizeof(float));
            first = last;
        }
        if (!directory.is_empty() && buffered_bytes > budget_bytes) {
            _spill();
        }
    }

    void MaszynaTrianglesSink::add_geometry_file(const String &p_path) {
        const Ref<MaszynaTrianglesSector> sector =
                ResourceLoader::get_singleton()->load(p_path, "", ResourceLoader::CACHE_MODE_IGNORE);
        ERR_FAIL_COND_MSG(sector.is_null(), "Cannot read a terrain sector: " + p_path);
        const TypedArray<MaszynaTrianglesChunkGeometry> sector_chunks = sector->get_chunks();
        for (int64_t index = 0; index < sector_chunks.size(); index++) {
            add_geometry(sector_chunks[index]);
        }
    }

    TypedArray<MaszynaTrianglesChunkGeometry> MaszynaTrianglesSink::get_geometries() const {
        MutexLock lock(mutex);
        TypedArray<MaszynaTrianglesChunkGeometry> geometries;
        for (const Chunk &chunk: chunks) {
            geometries.append(_to_geometry(chunk));
        }
        return geometries;
    }

    /// One section at a time: each of its chunks' parts read back and joined with what is still in
    /// memory, cut into pieces an upload each, and the section written as one file
    Array MaszynaTrianglesSink::finish() {
        MutexLock lock(mutex);
        ERR_FAIL_COND_V_MSG(directory.is_empty(), Array(), "A sink without a directory keeps its chunks in memory");
        // the chunks of each section, in the order they first appeared
        std::vector<Vector2i> sections;
        HashMap<Vector2i, std::vector<int>> section_chunks;
        for (int index = 0; index < static_cast<int>(chunks.size()); index++) {
            const Vector3 &origin = chunks[index].origin;
            const Vector2i section(
                    static_cast<int>(Math::floor(origin.x / CHUNK_SIZE_M)),
                    static_cast<int>(Math::floor(origin.z / CHUNK_SIZE_M)));
            if (!section_chunks.has(section)) {
                sections.push_back(section);
            }
            section_chunks[section].push_back(index);
        }
        Array descriptors;
        for (const Vector2i &section: sections) {
            const String path = directory.path_join(
                    String::num_int64(section.x) + String("_") + String::num_int64(section.y) + String(".res"));
            TypedArray<MaszynaTrianglesChunkGeometry> sector_chunks;
            Array sector_descriptors;
            for (const int index: section_chunks[section]) {
                Chunk &chunk = chunks[index];
                Chunk whole;
                whole.texture = chunk.texture;
                whole.origin = chunk.origin;
                whole.range_min = chunk.range_min;
                whole.range_max = chunk.range_max;
                for (const Group &group: chunk.groups) {
                    Group &copy = whole.groups.emplace_back();
                    copy.centre = group.centre;
                    copy.vertex_count = group.vertex_count;
                }
                for (int part = 0; part < chunk.parts; part++) {
                    const String part_path = _get_part_path(index, part);
                    const Ref<MaszynaTrianglesChunkGeometry> geometry =
                            ResourceLoader::get_singleton()->load(part_path, "", ResourceLoader::CACHE_MODE_IGNORE);
                    if (geometry.is_valid()) {
                        _append_part(whole, geometry);
                    }
                    DirAccess::remove_absolute(part_path);
                }
                for (size_t group = 0; group < chunk.groups.size(); group++) {
                    const Group &rest = chunk.groups[group];
                    Group &target = whole.groups[group];
                    target.vertices.insert(target.vertices.end(), rest.vertices.begin(), rest.vertices.end());
                    target.normals.insert(target.normals.end(), rest.normals.begin(), rest.normals.end());
                    target.uvs.insert(target.uvs.end(), rest.uvs.begin(), rest.uvs.end());
                }
                chunk = Chunk(); // its memory goes before the next chunk is read

                // a chunk is uploaded in one go when it is streamed: a large one goes as pieces
                const TypedArray<MaszynaTrianglesChunkGeometry> pieces = _to_geometry(whole)->split();
                for (int64_t piece = 0; piece < pieces.size(); piece++) {
                    Dictionary descriptor;
                    descriptor["path"] = path;
                    descriptor["index"] = sector_chunks.size();
                    descriptor["position"] = whole.origin;
                    descriptor["texture"] = whole.texture;
                    descriptor["range_min"] = whole.range_min;
                    descriptor["range_max"] = whole.range_max;
                    sector_descriptors.append(descriptor);
                    sector_chunks.append(pieces[piece]);
                }
            }
            Ref<MaszynaTrianglesSector> sector;
            sector.instantiate();
            sector->set_chunks(sector_chunks);
            if (ResourceCache::resource_save(sector, path) != OK) {
                UtilityFunctions::push_error("[MaszynaTrianglesSink] Cannot write a terrain sector to " + path);
                continue;
            }
            descriptors.append_array(sector_descriptors);
        }
        chunks.clear();
        chunk_indices.clear();
        buffered_bytes = 0;
        return descriptors;
    }
} // namespace godot
