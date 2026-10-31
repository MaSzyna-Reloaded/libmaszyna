// The code of the default material type, for its shader (default.gdshader) and its variants
// (default_*.gdshader, MaszynaMaterialFactory.SHADER_VARIANTS), each of them with its own
// render_mode and defines

#include "res://addons/libmaszyna/materials/types/weather.gdshaderinc"

#ifdef MASZYNA_ALPHA_BLEND
#include "res://addons/libmaszyna/materials/types/blended_reflection.gdshaderinc"
#endif

uniform vec4 albedo : source_color;
uniform sampler2D texture_albedo : source_color, filter_linear_mipmap, repeat_enable;
uniform int transparency = 0;
uniform float alpha_scissor_threshold : hint_range(0.0, 1.0) = 0.5;
uniform ivec2 albedo_texture_size;
uniform float point_size : hint_range(0.1, 128.0, 0.1);

uniform float roughness : hint_range(0.0, 1.0);
uniform sampler2D texture_metallic : hint_default_white, filter_linear_mipmap, repeat_enable;
uniform vec4 metallic_texture_channel;
uniform sampler2D texture_roughness : hint_roughness_r, filter_linear_mipmap, repeat_enable;

uniform float specular : hint_range(0.0, 1.0, 0.01);
uniform float metallic : hint_range(0.0, 1.0, 0.01);

uniform sampler2D texture_normal : hint_roughness_normal, filter_linear_mipmap, repeat_enable;
uniform float normal_scale : hint_range(-16.0, 16.0);

uniform bool emission_enabled = false;
// the submodel self-illuminates while the light outside is below its threshold
// (opengl33renderer.cpp:3452 - Global.fLuminance < fLight); 2.0 lights it always
uniform float lights_on_threshold = 2.0;
global uniform float reloaded_light_level;
// how bright self-illumination is drawn, of the setting (MaszynaMaterialManager)
global uniform float maszyna_selfillum_energy;
uniform vec4 emission_color : source_color = vec4(1.0);
uniform float emission_energy : hint_range(0.0, 16.0);

uniform vec3 uv1_scale;
uniform vec3 uv1_offset;
uniform vec3 uv2_scale;
uniform vec3 uv2_offset;

void vertex() {
	maszyna_weather_position = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
	UV = UV * uv1_scale.xy + uv1_offset.xy;
}

void fragment() {
	vec2 base_uv = UV;

	vec4 albedo_tex = texture(texture_albedo, base_uv);
#ifdef MASZYNA_ALPHA_BLEND
	ALPHA = albedo_tex.a;
#endif
	if (transparency == 2 && albedo_tex.a < alpha_scissor_threshold) {
		discard;
	}
	ALBEDO = albedo.rgb * albedo_tex.rgb;

	float metallic_tex = dot(texture(texture_metallic, base_uv), metallic_texture_channel);
	METALLIC = metallic_tex * metallic;
	SPECULAR = specular;

	vec4 roughness_texture_channel = vec4(1.0, 0.0, 0.0, 0.0);
	float roughness_tex = dot(texture(texture_roughness, base_uv), roughness_texture_channel);
	ROUGHNESS = roughness_tex * roughness;

	// Normal Map: Enabled
	NORMAL_MAP = texture(texture_normal, base_uv).rgb;
	NORMAL_MAP_DEPTH = normal_scale;

    if(emission_enabled && reloaded_light_level < lights_on_threshold) {
        EMISSION = emission_color.rgb * albedo.rgb * albedo_tex.rgb * emission_energy * maszyna_selfillum_energy;
    }

#ifdef MASZYNA_ALPHA_BLEND
	// the mask is the reflmap's alpha (mat_reflmap.frag:43), white without one
	SPECULAR = blended_specular(texture(texture_metallic, base_uv).a);
	ROUGHNESS = BLENDED_ROUGHNESS;
#ifdef MASZYNA_GLASS_LAYER
	MASZYNA_GLASS_LAYER(base_uv, texture_albedo);
#endif
#endif
#ifndef MASZYNA_ALPHA_BLEND
	maszyna_weather(ALBEDO, ROUGHNESS, NORMAL, NORMAL_MAP_DEPTH, INV_VIEW_MATRIX, VIEW_MATRIX, maszyna_weather_position);
#endif
}
