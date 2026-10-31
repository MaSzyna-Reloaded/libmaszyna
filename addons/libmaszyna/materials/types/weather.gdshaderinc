// The environment's weather on a material of the scenery: the snow lying on what faces up, the wet
// and the water standing in the hollows (libreloaded's ground_weather.gdshaderinc). A vehicle's and
// its cab's materials stay as they are (MaszynaMaterialManager.MaterialOptions.weather_exposed).

// absolute: the variants MaszynaMaterialFactory builds from this code have no path of their own
#include "res://addons/libreloaded/environment/ground_weather.gdshaderinc"

uniform bool weather_exposed = false;
// where the surface is in the world, set in vertex() (maszyna_weather_position = (MODEL_MATRIX *
// vec4(VERTEX, 1.0)).xyz): a double precision build keeps the view's origin in INV_VIEW_MATRIX its
// own way, and INV_VIEW_MATRIX * VERTEX was kilometres off far from the world's origin
varying vec3 maszyna_weather_position;

// How much of the rain stands on a surface of the scenery: a .mat says nothing of whether it is a
// paving or a soil, so somewhere between them
const float MASZYNA_WEATHER_PUDDLING = 0.3;

// The weather laid over what fragment() has made of the surface at world_position: its colour and
// roughness, its normal (view space) and how deep its normal map lies - the snow and the water
// cover its bumps
void maszyna_weather(inout vec3 surface_albedo, inout float surface_roughness, inout vec3 normal, inout float normal_map_depth,
		mat4 inv_view_matrix, mat4 view_matrix, vec3 world_position) {
	// nothing of it worked out while no weather lies on the surfaces, not even the normal turned
	if (!weather_exposed || !reloaded_weather_surfaces_active) {
		return;
	}
	// the heaps and drifts lie in the light only: the scenery's triangles are tens of metres across,
	// too coarse to be raised by them
	vec3 world_normal = mat3(inv_view_matrix) * normal;
	float covered = reloaded_weather_apply(surface_albedo, surface_roughness, world_normal, world_position, MASZYNA_WEATHER_PUDDLING);
	normal_map_depth *= 1.0 - covered;
	normal = normalize(mat3(view_matrix) * world_normal);
}

// maszyna_weather() on a surface with a normal map: the snow lies on what the map turns up - a
// brick's top, a sill - and the wind drives it onto what it turns into it. normal_map is NORMAL_MAP as
// fragment() set it, tangent and binormal TANGENT and BINORMAL; normal comes back the geometry's with
// the weather's turn only, Godot laying the map over it after fragment()
void maszyna_weather_mapped(inout vec3 surface_albedo, inout float surface_roughness, inout vec3 normal, inout float normal_map_depth,
		vec3 normal_map, vec3 tangent, vec3 binormal, mat4 inv_view_matrix, mat4 view_matrix, vec3 world_position) {
	// nothing of it worked out while no weather lies on the surfaces, not even the normal turned
	if (!weather_exposed || !reloaded_weather_surfaces_active) {
		return;
	}
	// as Godot lays NORMAL_MAP on NORMAL
	vec3 map = normal_map * 2.0 - 1.0;
	map.z = sqrt(max(0.0, 1.0 - dot(map.xy, map.xy)));
	vec3 mapped = normalize(mix(normal, tangent * map.x + binormal * map.y + normal * map.z, normal_map_depth));
	vec3 mapped_world = mat3(inv_view_matrix) * mapped;
	vec3 world_normal = mapped_world;
	float covered = reloaded_weather_apply(surface_albedo, surface_roughness, world_normal, world_position, MASZYNA_WEATHER_PUDDLING);
	normal_map_depth *= 1.0 - covered;
	normal = normalize(normal + mat3(view_matrix) * (world_normal - mapped_world));
}
