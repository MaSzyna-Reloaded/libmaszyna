#include "legacy/cabin/LegacyCabinLampIslands.hpp"

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/local_vector.hpp>

namespace godot {
    namespace {
        /// Pixels per side of a BC block
        constexpr int BC_BLOCK_SIDE = 4;
        constexpr int BC1_BLOCK_BYTES = 8;
        constexpr int BC3_BLOCK_BYTES = 16;
        /// Where the colour block of a DXT3/DXT5 block starts, after its alpha
        constexpr int BC_ALPHA_BYTES = 8;
        constexpr float BYTE_MAX = 255.0F;

        struct Piece {
                AABB bounds;
                Rect2 uv_bounds;
                Color color_sum = Color(0.0, 0.0, 0.0, 0.0);
                Color tint = Color(1.0, 1.0, 1.0);
        };
    } // namespace

    static int island_root(LocalVector<int> &p_parent, int p_vertex) {
        while (p_parent[p_vertex] != p_vertex) {
            p_parent[p_vertex] = p_parent[p_parent[p_vertex]];
            p_vertex = p_parent[p_vertex];
        }
        return p_vertex;
    }

    /// The gap between two pieces: their centres' distance less half of both longest sides
    static double island_gap(const AABB &p_first, const AABB &p_second) {
        return MAX(
                0.0, p_first.get_center().distance_to(p_second.get_center()) -
                             ((p_first.get_longest_axis_size() + p_second.get_longest_axis_size()) * 0.5));
    }

    /// Whether `p_candidate` at `p_candidate_gap` is nearer than `p_best` at `p_best_gap` - the lower
    /// index of two as near, as a scan in index order finds them
    static bool
    island_nearer(const double p_candidate_gap, const int p_candidate, const double p_best_gap, const int p_best) {
        return p_candidate_gap < p_best_gap || (p_candidate_gap == p_best_gap && p_candidate < p_best);
    }

    static bool has_color_channels(const Image::Format p_format) {
        switch (p_format) {
            case Image::FORMAT_L8:
            case Image::FORMAT_LA8:
            case Image::FORMAT_R8:
            case Image::FORMAT_RG8:
            case Image::FORMAT_RF:
            case Image::FORMAT_RGF:
            case Image::FORMAT_RH:
            case Image::FORMAT_RGH:
            case Image::FORMAT_RGTC_R:
            case Image::FORMAT_RGTC_RG:
            case Image::FORMAT_ETC2_R11:
            case Image::FORMAT_ETC2_R11S:
            case Image::FORMAT_ETC2_RG11:
            case Image::FORMAT_ETC2_RG11S:
                return false;
            default:
                return true;
        }
    }

    namespace {
        /// A pixel of the texture's full size, read where the samples fall: a DXT block is decoded
        /// alone, as Image::decompress() decodes it - a whole 4096 px texture decompressed for a few
        /// samples took tens of milliseconds a lamp. The integer formulas are bcdec's
        /// (thirdparty/misc/bcdec.h: bcdec__color_block, bcdec__sharp_alpha_block,
        /// bcdec__smooth_alpha_block), so a sample is the pixel the decompressed image has
        class TextureSampler {
            public:
                /// Nothing to read for a null image
                explicit TextureSampler(const Ref<Image> &p_image) {
                    if (p_image.is_null()) {
                        return;
                    }
                    format = p_image->get_format();
                    width = p_image->get_width();
                    height = p_image->get_height();
                    if (format == Image::FORMAT_DXT1 || format == Image::FORMAT_DXT3 || format == Image::FORMAT_DXT5) {
                        data = p_image->get_data();
                        return;
                    }
                    image = p_image;
                    if (image->is_compressed()) {
                        // the texture's own image is never changed
                        image = p_image->duplicate();
                        image->decompress();
                    }
                }

                int get_width() const {
                    return width;
                }

                int get_height() const {
                    return height;
                }

                Color get_pixel(const int p_x, const int p_y) const {
                    if (image.is_valid()) {
                        return image->get_pixel(p_x, p_y);
                    }
                    const int blocks_per_row = MAX(1, (width + BC_BLOCK_SIDE - 1) / BC_BLOCK_SIDE);
                    const int block_bytes = format == Image::FORMAT_DXT1 ? BC1_BLOCK_BYTES : BC3_BLOCK_BYTES;
                    const int64_t block =
                            static_cast<int64_t>(((p_y / BC_BLOCK_SIDE) * blocks_per_row) + (p_x / BC_BLOCK_SIDE)) *
                            block_bytes;
                    const int pixel = ((p_y % BC_BLOCK_SIDE) * BC_BLOCK_SIDE) + (p_x % BC_BLOCK_SIDE);
                    if (format == Image::FORMAT_DXT1) {
                        return _color(block, pixel, false, static_cast<int>(BYTE_MAX));
                    }
                    int alpha = 0;
                    if (format == Image::FORMAT_DXT3) {
                        // four bits a pixel
                        alpha = ((data[block + (pixel / 2)] >> (4 * (pixel % 2))) & 0xF) * 17;
                    } else {
                        // two ends and three bits a pixel
                        const int alpha0 = data[block];
                        const int alpha1 = data[block + 1];
                        uint64_t indices = 0;
                        for (int byte = 0; byte < 6; byte++) {
                            indices |= static_cast<uint64_t>(data[block + 2 + byte]) << (8 * byte);
                        }
                        const int index = static_cast<int>((indices >> (3 * pixel)) & 0x7);
                        if (index == 0) {
                            alpha = alpha0;
                        } else if (index == 1) {
                            alpha = alpha1;
                        } else if (alpha0 > alpha1) {
                            alpha = (((8 - index) * alpha0) + ((index - 1) * alpha1) + 1) / 7;
                        } else if (index < 6) {
                            alpha = (((6 - index) * alpha0) + ((index - 1) * alpha1) + 1) / 5;
                        } else {
                            alpha = index == 6 ? 0 : static_cast<int>(BYTE_MAX);
                        }
                    }
                    return _color(block + BC_ALPHA_BYTES, pixel, true, alpha);
                }

            private:
                /// The colour block at `p_offset`: two RGB565 ends, two bits a pixel; DXT1 without
                /// `p_opaque` has a transparent black fourth colour when the first end is not the
                /// greater
                Color _color(const int64_t p_offset, const int p_pixel, const bool p_opaque, const int p_alpha) const {
                    const int color0 = data[p_offset] | (data[p_offset + 1] << 8);
                    const int color1 = data[p_offset + 2] | (data[p_offset + 3] << 8);
                    const uint32_t indices = data[p_offset + 4] | (data[p_offset + 5] << 8) |
                                             (data[p_offset + 6] << 16) |
                                             (static_cast<uint32_t>(data[p_offset + 7]) << 24);
                    const int red0 = ((((color0 >> 11) & 0x1F) * 527) + 23) >> 6;
                    const int green0 = ((((color0 >> 5) & 0x3F) * 259) + 33) >> 6;
                    const int blue0 = (((color0 & 0x1F) * 527) + 23) >> 6;
                    const int red1 = ((((color1 >> 11) & 0x1F) * 527) + 23) >> 6;
                    const int green1 = ((((color1 >> 5) & 0x3F) * 259) + 33) >> 6;
                    const int blue1 = (((color1 & 0x1F) * 527) + 23) >> 6;
                    const bool four_colors = color0 > color1 || p_opaque;
                    int red = red0;
                    int green = green0;
                    int blue = blue0;
                    switch ((indices >> (2 * p_pixel)) & 0x3) {
                        case 1:
                            red = red1;
                            green = green1;
                            blue = blue1;
                            break;
                        case 2:
                            red = four_colors ? ((((2 * red0) + red1) * 351) + 61) >> 10 : (red0 + red1 + 1) >> 1;
                            green = four_colors ? ((((2 * green0) + green1) * 351) + 61) >> 10
                                                : (green0 + green1 + 1) >> 1;
                            blue = four_colors ? ((((2 * blue0) + blue1) * 351) + 61) >> 10 : (blue0 + blue1 + 1) >> 1;
                            break;
                        case 3:
                            if (!four_colors) {
                                return {0.0, 0.0, 0.0, 0.0};
                            }
                            red = (((red0 + (2 * red1)) * 351) + 61) >> 10;
                            green = (((green0 + (2 * green1)) * 351) + 61) >> 10;
                            blue = (((blue0 + (2 * blue1)) * 351) + 61) >> 10;
                            break;
                        default:
                            break;
                    }
                    return {static_cast<float>(red) / BYTE_MAX, static_cast<float>(green) / BYTE_MAX,
                            static_cast<float>(blue) / BYTE_MAX, static_cast<float>(p_alpha) / BYTE_MAX};
                }

                Image::Format format = Image::FORMAT_RGBA8;
                int width = 0;
                int height = 0;
                PackedByteArray data;
                Ref<Image> image;
        };

        /// The pieces' centres in cells of a grid, so a piece's nearest is looked for around it, not
        /// among all of them - a dashboard light of 7237 pieces took seconds measured against all.
        /// A piece grown longer than LARGE_PIECE_CELLS cells (the lamp merged so far) is kept apart
        /// and always looked at, so it does not widen every search
        class PieceGrid {
            public:
                static constexpr double LARGE_PIECE_CELLS = 4.0;

                PieceGrid(const LocalVector<Piece> &p_pieces, const LocalVector<bool> &p_alive) :
                    pieces(p_pieces), alive(p_alive) {
                    // a cell holds about one piece: the spread of their centres over the two
                    // longest sides (a lamp mesh lies on a panel) shared among them
                    double longest_sum = 0.0;
                    AABB spread;
                    for (uint32_t piece = 0; piece < pieces.size(); piece++) {
                        longest_sum += pieces[piece].bounds.get_longest_axis_size();
                        if (piece == 0) {
                            spread = AABB(pieces[piece].bounds.get_center(), Vector3());
                        }
                        spread.expand_to(pieces[piece].bounds.get_center());
                    }
                    const Vector3 extent = spread.size;
                    const double panel_area = (extent.x * extent.y) + (extent.y * extent.z) + (extent.z * extent.x);
                    cell_size = pieces.is_empty()
                                        ? LegacyCabinLampIslands::MERGE_DISTANCE
                                        : MAX(MAX(LegacyCabinLampIslands::MERGE_DISTANCE, longest_sum / pieces.size()),
                                              Math::sqrt(panel_area / pieces.size()));
                    alive_count = static_cast<int>(pieces.size());
                    for (int piece = 0; piece < static_cast<int>(pieces.size()); piece++) {
                        insert(piece);
                    }
                }

                void insert(const int p_piece) {
                    if (_is_large(p_piece)) {
                        large.push_back(p_piece);
                        return;
                    }
                    const Vector3i cell = _cell_of(p_piece);
                    if (cells.is_empty()) {
                        cell_min = cell;
                        cell_max = cell;
                    }
                    cell_min = cell_min.min(cell);
                    cell_max = cell_max.max(cell);
                    cells[cell].push_back(p_piece);
                }

                /// Before the piece's bounds change, and when it is merged away
                void remove(const int p_piece) {
                    if (_is_large(p_piece)) {
                        large.erase(p_piece);
                        return;
                    }
                    cells[_cell_of(p_piece)].erase(p_piece);
                }

                void set_alive_count(const int p_alive_count) {
                    alive_count = p_alive_count;
                }

                /// The nearest live piece to `p_piece` and the gap to it (-1 and INF alone); a piece
                /// touching it (gap 0) is as near as any
                void find_nearest(const int p_piece, int &p_nearest, double &p_nearest_gap) const {
                    p_nearest = -1;
                    p_nearest_gap = Math::INF;
                    for (const int other: large) {
                        _consider(p_piece, other, p_nearest, p_nearest_gap);
                    }
                    const double reach_size =
                            (pieces[p_piece].bounds.get_longest_axis_size() + (LARGE_PIECE_CELLS * cell_size)) * 0.5;
                    const Vector3i center = _cell_of(p_piece);
                    const Vector3i to_min = center - cell_min;
                    const Vector3i to_max = cell_max - center;
                    const int reach =
                            MAX(MAX(MAX(to_min.x, to_max.x), MAX(to_min.y, to_max.y)), MAX(to_min.z, to_max.z));
                    for (int ring = 0; ring <= reach && p_nearest_gap > 0.0; ring++) {
                        // any centre in this ring is at least (ring - 1) cells away
                        if (ring > 0 && ((ring - 1) * cell_size) - reach_size > p_nearest_gap) {
                            return;
                        }
                        const int side = (2 * ring) + 1;
                        // a search wider than the pieces left is a plain scan of them
                        if (static_cast<int64_t>(side) * side * side > alive_count) {
                            for (int other = 0; other < static_cast<int>(pieces.size()) && p_nearest_gap > 0.0;
                                 other++) {
                                _consider(p_piece, other, p_nearest, p_nearest_gap);
                            }
                            return;
                        }
                        for (int x = -ring; x <= ring; x++) {
                            for (int y = -ring; y <= ring; y++) {
                                for (int z = -ring; z <= ring; z++) {
                                    if (MAX(MAX(Math::abs(x), Math::abs(y)), Math::abs(z)) < ring) {
                                        continue;
                                    }
                                    const LocalVector<int> *cell = cells.getptr(center + Vector3i(x, y, z));
                                    if (cell == nullptr) {
                                        continue;
                                    }
                                    for (const int other: *cell) {
                                        _consider(p_piece, other, p_nearest, p_nearest_gap);
                                    }
                                }
                            }
                        }
                    }
                }

            private:
                bool _is_large(const int p_piece) const {
                    return pieces[p_piece].bounds.get_longest_axis_size() > LARGE_PIECE_CELLS * cell_size;
                }

                Vector3i _cell_of(const int p_piece) const {
                    const Vector3 center = pieces[p_piece].bounds.get_center() / cell_size;
                    return Vector3i(
                            static_cast<int>(Math::floor(center.x)), static_cast<int>(Math::floor(center.y)),
                            static_cast<int>(Math::floor(center.z)));
                }

                void _consider(const int p_piece, const int p_other, int &p_nearest, double &p_nearest_gap) const {
                    if (p_other == p_piece || !alive[p_other]) {
                        return;
                    }
                    const double gap = island_gap(pieces[p_piece].bounds, pieces[p_other].bounds);
                    if (island_nearer(gap, p_other, p_nearest_gap, p_nearest)) {
                        p_nearest_gap = gap;
                        p_nearest = p_other;
                    }
                }

                const LocalVector<Piece> &pieces;
                const LocalVector<bool> &alive;
                HashMap<Vector3i, LocalVector<int>> cells;
                LocalVector<int> large;
                Vector3i cell_min;
                Vector3i cell_max;
                double cell_size = 0.0;
                int alive_count = 0;
        };
    } // namespace

    void LegacyCabinLampIslands::_bind_methods() {
        ClassDB::bind_static_method(
                "LegacyCabinLampIslands", D_METHOD("submodel_islands", "lamp"),
                &LegacyCabinLampIslands::submodel_islands);
    }

    TypedArray<Dictionary> LegacyCabinLampIslands::submodel_islands(Node3D *p_lamp) {
        TypedArray<Dictionary> islands;
        ERR_FAIL_NULL_V(p_lamp, islands);
        TypedArray<Node> mesh_instances = p_lamp->find_children("", "MeshInstance3D", true, false);
        if (Object::cast_to<MeshInstance3D>(p_lamp) != nullptr) {
            mesh_instances.append(p_lamp);
        }

        LocalVector<Piece> pieces;
        for (int node = 0; node < mesh_instances.size(); node++) {
            const MeshInstance3D *mesh_instance = Object::cast_to<MeshInstance3D>(mesh_instances[node]);
            const Ref<Mesh> mesh = mesh_instance->get_mesh();
            if (mesh.is_null()) {
                continue;
            }
            Color tint(1.0, 1.0, 1.0);
            Ref<Image> texture_image;
            const Ref<ShaderMaterial> material = mesh_instance->get_material_override();
            if (material.is_valid()) {
                const Variant albedo = material->get_shader_parameter("albedo");
                if (albedo.get_type() == Variant::COLOR) {
                    tint = albedo;
                }
                const Ref<Texture2D> texture = material->get_shader_parameter("texture_albedo");
                texture_image = texture.is_valid() ? texture->get_image() : Ref<Image>();
            }
            // only a texture with colour channels says what colour the lamp is (SM42's lamps are
            // two-channel BC5) - otherwise the tint alone does
            const bool sampled = texture_image.is_valid() && has_color_channels(texture_image->get_format());
            const TextureSampler sampler(sampled ? texture_image : Ref<Image>());
            const Transform3D transform = mesh_instance->get_global_transform();
            for (int surface = 0; surface < mesh->get_surface_count(); surface++) {
                const Array arrays = mesh->surface_get_arrays(surface);
                const PackedVector3Array vertices = arrays[Mesh::ARRAY_VERTEX];
                const PackedVector2Array uvs = arrays[Mesh::ARRAY_TEX_UV];
                PackedInt32Array indices = arrays[Mesh::ARRAY_INDEX];
                if (indices.is_empty()) {
                    indices.resize(vertices.size());
                    for (int vertex = 0; vertex < vertices.size(); vertex++) {
                        indices[vertex] = vertex;
                    }
                }
                // union-find over the vertices of each triangle
                LocalVector<int> parent;
                parent.resize(vertices.size());
                for (int vertex = 0; vertex < vertices.size(); vertex++) {
                    parent[vertex] = vertex;
                }
                for (int triangle = 0; triangle + 2 < indices.size(); triangle += 3) {
                    for (int corner = 1; corner <= 2; corner++) {
                        const int first = island_root(parent, indices[triangle]);
                        const int second = island_root(parent, indices[triangle + corner]);
                        if (first != second) {
                            parent[first] = second;
                        }
                    }
                }
                HashMap<int, uint32_t> piece_of_root;
                const uint32_t surface_start = pieces.size();
                for (int vertex = 0; vertex < vertices.size(); vertex++) {
                    const int root = island_root(parent, vertex);
                    const Vector3 global_vertex = transform.xform(vertices[vertex]);
                    const Vector2 uv = uvs.is_empty() ? Vector2() : uvs[vertex];
                    if (!piece_of_root.has(root)) {
                        piece_of_root[root] = pieces.size();
                        Piece piece;
                        piece.bounds = AABB(global_vertex, Vector3());
                        piece.uv_bounds = Rect2(uv, Vector2());
                        piece.tint = tint;
                        pieces.push_back(piece);
                    }
                    Piece &piece = pieces[piece_of_root[root]];
                    piece.bounds.expand_to(global_vertex);
                    piece.uv_bounds.expand_to(uv);
                }
                if (!sampled) {
                    continue;
                }
                const int width = sampler.get_width();
                const int height = sampler.get_height();
                for (uint32_t index = surface_start; index < pieces.size(); index++) {
                    Piece &piece = pieces[index];
                    for (int sample_x = 0; sample_x < COLOR_SAMPLES; sample_x++) {
                        for (int sample_y = 0; sample_y < COLOR_SAMPLES; sample_y++) {
                            const Vector2 uv = piece.uv_bounds.position +
                                               piece.uv_bounds.size * Vector2((sample_x + 0.5) / COLOR_SAMPLES,
                                                                              (sample_y + 0.5) / COLOR_SAMPLES);
                            const Color pixel = sampler.get_pixel(
                                    static_cast<int>(Math::posmod(
                                            static_cast<int64_t>(uv.x * width), static_cast<int64_t>(width))),
                                    static_cast<int>(Math::posmod(
                                            static_cast<int64_t>(uv.y * height), static_cast<int64_t>(height))));
                            piece.color_sum += Color(pixel.r, pixel.g, pixel.b, 1.0) * pixel.a;
                        }
                    }
                }
            }
        }

        const int count = static_cast<int>(pieces.size());
        LocalVector<bool> alive;
        alive.resize(count);
        for (int piece = 0; piece < count; piece++) {
            alive[piece] = true;
        }
        if (count > BACKLIGHT_PIECE_COUNT) {
            // the workaround for a backlight modelled as one lamp: one light at all of it
            for (int piece = 1; piece < count; piece++) {
                pieces[0].bounds.merge_with(pieces[piece].bounds);
                pieces[0].color_sum += pieces[piece].color_sum;
                alive[piece] = false;
            }
        } else {
            // pieces nearer than MERGE_DISTANCE, then the nearest while there are too many: always
            // the nearest pair of all, as measuring every pair after every merge finds it. Every
            // piece keeps its nearest, so a merge looks again only at what it changed
            PieceGrid grid(pieces, alive);
            LocalVector<int> nearest;
            nearest.resize(count);
            LocalVector<double> nearest_gap;
            nearest_gap.resize(count);
            for (int piece = 0; piece < count; piece++) {
                grid.find_nearest(piece, nearest[piece], nearest_gap[piece]);
            }
            int alive_count = count;
            while (alive_count > 1) {
                int kept = -1;
                // none is nearer than touching
                for (int piece = 0; piece < count && (kept < 0 || nearest_gap[kept] > 0.0); piece++) {
                    if (alive[piece] && (kept < 0 || nearest_gap[piece] < nearest_gap[kept])) {
                        kept = piece;
                    }
                }
                if (nearest_gap[kept] >= MERGE_DISTANCE && alive_count <= LIGHT_MAX_COUNT) {
                    break;
                }
                const int gone = nearest[kept];
                grid.remove(kept);
                grid.remove(gone);
                pieces[kept].bounds.merge_with(pieces[gone].bounds);
                pieces[kept].color_sum += pieces[gone].color_sum;
                alive[gone] = false;
                alive_count--;
                grid.set_alive_count(alive_count);
                grid.insert(kept);
                // only the grown piece changed: it is the nearest of whoever it is now nearer to than
                // that one's nearest; who had either merged piece nearest and is not nearer to the grown
                // one looks again
                grid.find_nearest(kept, nearest[kept], nearest_gap[kept]);
                for (int piece = 0; piece < count; piece++) {
                    if (!alive[piece] || piece == kept) {
                        continue;
                    }
                    const bool lost_nearest = nearest[piece] == kept || nearest[piece] == gone;
                    // a piece touching its nearest has none nearer
                    if (!lost_nearest && nearest_gap[piece] == 0.0) {
                        continue;
                    }
                    const double gap = island_gap(pieces[piece].bounds, pieces[kept].bounds);
                    if (lost_nearest ? gap <= nearest_gap[piece]
                                     : island_nearer(gap, kept, nearest_gap[piece], nearest[piece])) {
                        nearest_gap[piece] = gap;
                        nearest[piece] = kept;
                    } else if (lost_nearest) {
                        grid.find_nearest(piece, nearest[piece], nearest_gap[piece]);
                    }
                }
            }
        }

        for (int index = 0; index < count; index++) {
            if (!alive[index]) {
                continue;
            }
            const Piece &piece = pieces[index];
            Color color = piece.tint;
            if (piece.color_sum.a > 0.0) {
                color = Color(piece.color_sum.r / piece.color_sum.a, piece.color_sum.g / piece.color_sum.a,
                              piece.color_sum.b / piece.color_sum.a) *
                        piece.tint;
            }
            const float brightest = MAX(color.r, MAX(color.g, color.b));
            if (brightest > 0.0) {
                color = Color(color.r / brightest, color.g / brightest, color.b / brightest);
            }
            Dictionary island;
            island["position"] = piece.bounds.get_center();
            island["color"] = color;
            islands.append(island);
        }
        return islands;
    }
} // namespace godot
