// A ring of a heightmap terrain's clipmap (MaszynaHeightmapTerrainRenderingServer): a grid of
// CELLS x CELLS cells of ring_spacing, centred on the camera, its points raised to the heights of the
// terrain's chunks under them and painted with up to 8 layers of the terrain's palette, blended by
// their weights (mat_terrain.frag). Near its outer edge a ring's points move onto the grid of the ring
// around it, so the two meet without a gap.
//
// The terrain's data are textures, a region of 1 km each (REGION_SIZE):
//   region_map: the slot of each region afar (R) and near (G), 0 for none
//   coarse_heights / fine_heights: the heights of a region afar / near
//   chunk_info: per chunk of a region afar, CHUNK_INFO texels in a row - the palette indices of its
//               first four layers and of the next four (255: none; the first: no chunk, a hole), the
//               average weights of the first four and of the next four
//   paint_info: per chunk of a region near, its slot in paint_weights0 / paint_weights1, -1 none
//   palette: per layer of the palette - row 0 its index in the layer arrays, 1 / metres its
//            textures repeat at, has a normal map, has a specular/gloss map; row 1 its reflection,
//            the cosine and the sine of the angle its textures are turned by, its glossiness
shader_type spatial;
render_mode cull_back, depth_draw_opaque;

#include "res://addons/libmaszyna/materials/types/glossiness.gdshaderinc"

const float REGION_SIZE = 1000.0;
const float CHUNK_SIZE = 250.0;
const int REGION_CHUNKS = 4;
const int CHUNK_INFO = 4;
const int COARSE_TEXELS = 65;
const int CELLS = 128;
// a ring's points move onto the next ring's grid over these cells, done this many cells before the
// next ring's hole may begin - it stands up to two of this ring's cells off the camera
const float MORPH_CELLS = 8.0;
const float MORPH_DONE_CELLS = 7.0;
// the textures of the layers repeat over a pack of chunks, 16 chunks a side (heightmap::pack_side)
const float PACK_SIZE = 4000.0;
// the paint of a chunk, its samples along a side less one
const float PAINT_SAMPLES = 250.0;
const float NO_LAYER = 254.5;
// no weight below this (mat_terrain.frag:75-76)
const float NO_WEIGHT = 0.004;
// a point of no chunk is left out: its triangles are not drawn
const float HOLE_PROBE = 0.01;

uniform float ring_spacing = 2.0;
uniform bool ring_coarse = false;
uniform bool next_coarse = false;
uniform bool ring_last = false;
uniform sampler2D region_map : filter_nearest;
uniform vec2 region_origin;
uniform sampler2DArray coarse_heights : filter_nearest;
uniform sampler2DArray chunk_info : filter_nearest;
uniform sampler2DArray fine_heights : filter_nearest;
uniform int fine_texels = 501;
uniform sampler2DArray paint_info : filter_nearest;
uniform sampler2DArray paint_weights0 : filter_linear, repeat_disable;
uniform sampler2DArray paint_weights1 : filter_linear, repeat_disable;
uniform sampler2D palette : filter_nearest;
uniform sampler2DArray layer_diffuse : source_color, filter_linear_mipmap_anisotropic, repeat_enable;
uniform sampler2DArray layer_normal : filter_linear_mipmap_anisotropic, repeat_enable;
uniform sampler2DArray layer_specgloss : filter_linear_mipmap_anisotropic, repeat_enable;

varying vec2 terrain_position;

ivec2 region_of(vec2 point) {
	return ivec2(floor((point - region_origin) / REGION_SIZE));
}

// the region's slots afar and near, plus one; 0 for none
vec2 region_slots(ivec2 region) {
	ivec2 size = textureSize(region_map, 0);
	if (any(lessThan(region, ivec2(0))) || any(greaterThanEqual(region, size))) {
		return vec2(0.0);
	}
	return texelFetch(region_map, region, 0).rg;
}

ivec2 chunk_in_region(vec2 point, ivec2 region) {
	vec2 local = point - region_origin - vec2(region) * REGION_SIZE;
	return clamp(ivec2(floor(local / CHUNK_SIZE)), ivec2(0), ivec2(REGION_CHUNKS - 1));
}

bool chunk_present(vec2 point) {
	ivec2 region = region_of(point);
	vec2 slots = region_slots(region);
	if (slots.x < 0.5) {
		return false;
	}
	ivec2 chunk = chunk_in_region(point, region);
	float first = texelFetch(chunk_info, ivec3(chunk.x * CHUNK_INFO, chunk.y, int(slots.x) - 1), 0).r * 255.0;
	return first < NO_LAYER;
}

// the height of the region's surface at the point, of its heights afar or near
float region_height(vec2 point, bool coarse) {
	ivec2 region = region_of(point);
	vec2 slots = region_slots(region);
	bool fine = !coarse && slots.y > 0.5;
	if (!fine && slots.x < 0.5) {
		return 0.0;
	}
	int texels = fine ? fine_texels : COARSE_TEXELS;
	vec2 local = (point - region_origin - vec2(region) * REGION_SIZE) / (REGION_SIZE / float(texels - 1));
	ivec2 at = clamp(ivec2(floor(local)), ivec2(0), ivec2(texels - 2));
	vec2 f = clamp(local - vec2(at), 0.0, 1.0);
	float h00;
	float h10;
	float h01;
	float h11;
	if (fine) {
		int layer = int(slots.y) - 1;
		h00 = texelFetch(fine_heights, ivec3(at, layer), 0).r;
		h10 = texelFetch(fine_heights, ivec3(at + ivec2(1, 0), layer), 0).r;
		h01 = texelFetch(fine_heights, ivec3(at + ivec2(0, 1), layer), 0).r;
		h11 = texelFetch(fine_heights, ivec3(at + ivec2(1, 1), layer), 0).r;
	} else {
		int layer = int(slots.x) - 1;
		h00 = texelFetch(coarse_heights, ivec3(at, layer), 0).r;
		h10 = texelFetch(coarse_heights, ivec3(at + ivec2(1, 0), layer), 0).r;
		h01 = texelFetch(coarse_heights, ivec3(at + ivec2(0, 1), layer), 0).r;
		h11 = texelFetch(coarse_heights, ivec3(at + ivec2(1, 1), layer), 0).r;
	}
	return mix(mix(h00, h10, f.x), mix(h01, h11, f.x), f.y);
}

// the ring's height, moving to the next ring's as the point moves onto its grid
float terrain_height(vec2 point, float morph) {
	float own = region_height(point, ring_coarse);
	return morph > 0.0 ? mix(own, region_height(point, next_coarse), morph) : own;
}

void vertex() {
	vec3 camera = MAIN_CAM_INV_VIEW_MATRIX[3].xyz;
	float spacing = ring_spacing;
	// the centre on the next ring's grid, so this ring's even points are that ring's points
	vec2 centre = round(camera.xz / (2.0 * spacing)) * 2.0 * spacing;
	vec2 grid = VERTEX.xz;
	vec2 point = centre + grid * spacing;
	float half_cells = float(CELLS / 2);
	float distance_out = max(abs(point.x - camera.x), abs(point.y - camera.z));
	float morph = ring_last ? 0.0 : clamp(
			(distance_out - (half_cells - MORPH_DONE_CELLS - MORPH_CELLS) * spacing) / (MORPH_CELLS * spacing), 0.0, 1.0);
	point -= mod(grid, 2.0) * spacing * morph;
	if (!(chunk_present(point + vec2(HOLE_PROBE)) || chunk_present(point - vec2(HOLE_PROBE))
			|| chunk_present(point + vec2(HOLE_PROBE, -HOLE_PROBE)) || chunk_present(point + vec2(-HOLE_PROBE, HOLE_PROBE)))) {
		VERTEX = vec3(intBitsToFloat(0x7fc00000));
	} else {
		float height = terrain_height(point, morph);
		float left = terrain_height(point - vec2(spacing, 0.0), morph);
		float right = terrain_height(point + vec2(spacing, 0.0), morph);
		float back = terrain_height(point - vec2(0.0, spacing), morph);
		float front = terrain_height(point + vec2(0.0, spacing), morph);
		VERTEX = vec3(point.x, height, point.y);
		NORMAL = normalize(vec3(left - right, 2.0 * spacing, back - front));
		// the textures' u along x, their v along -z
		TANGENT = normalize(vec3(2.0 * spacing, right - left, 0.0));
		BINORMAL = normalize(cross(NORMAL, TANGENT));
	}
	terrain_position = point;
}

void fragment() {
	vec2 point = terrain_position;
	ivec2 region = region_of(point);
	vec2 slots = region_slots(region);
	if (slots.x < 0.5) {
		discard;
	}
	ivec2 chunk = chunk_in_region(point, region);
	int info_layer = int(slots.x) - 1;
	vec4 layers0 = texelFetch(chunk_info, ivec3(chunk.x * CHUNK_INFO, chunk.y, info_layer), 0) * 255.0;
	vec4 layers1 = texelFetch(chunk_info, ivec3(chunk.x * CHUNK_INFO + 1, chunk.y, info_layer), 0) * 255.0;
	if (layers0.x > NO_LAYER) {
		discard;
	}
	vec4 w0 = texelFetch(chunk_info, ivec3(chunk.x * CHUNK_INFO + 2, chunk.y, info_layer), 0);
	vec4 w1 = texelFetch(chunk_info, ivec3(chunk.x * CHUNK_INFO + 3, chunk.y, info_layer), 0);
	// near, a painted chunk's own weights; afar their averages
	if (slots.y > 0.5) {
		float paint_slot = texelFetch(paint_info, ivec3(chunk, int(slots.y) - 1), 0).r;
		if (paint_slot >= 0.0) {
			vec2 local = (point - region_origin - vec2(region) * REGION_SIZE - vec2(chunk) * CHUNK_SIZE) / CHUNK_SIZE;
			vec3 paint_coord = vec3((local * PAINT_SAMPLES + 0.5) / (PAINT_SAMPLES + 1.0), paint_slot);
			w0 = texture(paint_weights0, paint_coord);
			w1 = texture(paint_weights1, paint_coord);
		}
	}
	float weights[8] = float[8](w0.r, w0.g, w0.b, w0.a, w1.r, w1.g, w1.b, w1.a);
	float indices[8] = float[8](layers0.r, layers0.g, layers0.b, layers0.a, layers1.r, layers1.g, layers1.b, layers1.a);
	float painted = dot(w0, vec4(1.0)) + dot(w1, vec4(1.0));

	// metres from the corner of the pack, along x and -z (heightmap::build_mesh())
	vec2 pack_corner = floor(point / PACK_SIZE) * PACK_SIZE;
	vec2 coord_m = vec2(point.x - pack_corner.x, pack_corner.y - point.y);
	vec2 coord_dx = dFdx(coord_m);
	vec2 coord_dy = dFdy(coord_m);

	vec3 albedo = vec3(0.0);
	vec3 normal = vec3(0.0);
	float reflectivity = 0.0;
	float specularity = 0.0;
	float gloss = 0.0;
	float metal = 0.0;
	float total = 0.0;
	for (int i = 0; i < 8; ++i) {
		if (indices[i] > NO_LAYER) {
			continue;
		}
		// a chunk without weights shows its first layer
		float weight = painted < NO_WEIGHT ? (i == 0 ? 1.0 : 0.0) : weights[i];
		if (weight < NO_WEIGHT) {
			continue;
		}
		int index = int(round(indices[i]));
		vec4 info = texelFetch(palette, ivec2(index, 0), 0);
		vec4 extra = texelFetch(palette, ivec2(index, 1), 0);
		// turned counterclockwise seen from above; the coordinates run along x and -z, so that is
		// clockwise in them
		vec2 turn = (extra.y == 0.0 && extra.z == 0.0) ? vec2(1.0, 0.0) : extra.yz;
		mat2 rotation = mat2(vec2(turn.x, -turn.y), vec2(turn.y, turn.x));
		vec3 coord = vec3(rotation * coord_m * info.y, info.x);
		vec2 dx = rotation * coord_dx * info.y;
		vec2 dy = rotation * coord_dy * info.y;
		albedo += textureGrad(layer_diffuse, coord, dx, dy).rgb * weight;
		vec3 layer_normal_ts = vec3(0.0, 0.0, 1.0);
		float layer_reflection = extra.x;
		if (info.z > 0.5) {
			vec4 normal_map = textureGrad(layer_normal, coord, dx, dy);
			layer_normal_ts.xy = normal_map.rg * 2.0 - 1.0;
			layer_normal_ts.z = sqrt(1.0 - clamp(dot(layer_normal_ts.xy, layer_normal_ts.xy), 0.0, 1.0));
			// the normal map leans along the turned texture; turned back to the axes of the terrain
			layer_normal_ts.xy = layer_normal_ts.xy * rotation;
			layer_reflection *= normal_map.a;
		}
		normal += layer_normal_ts * weight;
		if (info.w > 0.5) {
			vec4 specgloss = textureGrad(layer_specgloss, coord, dx, dy);
			specularity += specgloss.r * weight;
			gloss += specgloss.g * extra.w * weight;
			metal += specgloss.b * weight;
		} else {
			gloss += extra.w * weight;
		}
		reflectivity += layer_reflection * weight;
		total += weight;
	}
	total = max(total, NO_WEIGHT);
	ALBEDO = albedo / total;
	vec3 blended = normalize(normal);
	NORMAL_MAP = vec3(blended.xy * 0.5 + 0.5, 1.0);
	SPECULAR = clamp(max(specularity, reflectivity) / total, 0.0, 1.0);
	METALLIC = clamp(metal / total, 0.0, 1.0);
	ROUGHNESS = glossiness_to_roughness(gloss / total);
}
