shader_type spatial;

render_mode blend_mix, depth_prepass_alpha, cull_back, diffuse_burley, specular_schlick_ggx;

uniform sampler2D diffuse_texture : source_color, filter_linear_mipmap_anisotropic, repeat_enable;
uniform sampler2D normal_texture : filter_linear_mipmap_anisotropic, repeat_enable;
uniform sampler2D dudv_texture : filter_linear_mipmap_anisotropic, repeat_enable;

uniform vec4 material_color = vec4(1.0, 1.0, 1.0, 0.0);
uniform float reflection_strength : hint_range(0.0, 1.0) = 0.0;
uniform float glossiness = 10.0;
uniform float wave_strength : hint_range(0.0, 1.0) = 0.05;
uniform float wave_speed : hint_range(0.0, 1.0) = 0.02;

varying vec3 v_tangent_view;
varying vec3 v_binormal_view;
varying vec3 v_normal_view;
varying vec3 v_position_view;

float glossiness_to_roughness(float source_glossiness) {
    if (source_glossiness <= 0.0) {
        return 0.6;
    }

    return clamp(sqrt(2.0 / (source_glossiness + 2.0)), 0.03, 1.0);
}

vec3 decode_normal_from_rg(vec4 normal_sample) {
    vec3 n;

    n.xy = normal_sample.rg * 2.0 - 1.0;
    n.z = sqrt(1.0 - clamp(dot(n.xy, n.xy), 0.0, 1.0));

    return n * 0.5 + 0.5;
}

vec2 distort_uv(vec2 uv) {
    float move_factor = mod(TIME * wave_speed, 1.0);
    vec2 distorted_uv = texture(dudv_texture, vec2(uv.x + move_factor, uv.y)).rg * 0.1;

    distorted_uv = uv + vec2(distorted_uv.x, distorted_uv.y + move_factor);

    vec2 total_distortion = (texture(dudv_texture, distorted_uv).rg * 2.0 - 1.0) * wave_strength;

    return uv + total_distortion;
}

void vertex() {
    v_tangent_view = normalize(MODELVIEW_NORMAL_MATRIX * TANGENT);
    v_binormal_view = normalize(MODELVIEW_NORMAL_MATRIX * -BINORMAL);
    v_normal_view = normalize(MODELVIEW_NORMAL_MATRIX * NORMAL);
    v_position_view = (MODELVIEW_MATRIX * vec4(VERTEX, 1.0)).xyz;
}

void fragment() {
    vec2 water_uv = distort_uv(UV);
    vec4 diffuse_sample = texture(diffuse_texture, water_uv);
    vec4 normal_sample = texture(normal_texture, water_uv);

    vec3 view_dir = normalize(-v_position_view);
    float facing = clamp(dot(normalize(v_normal_view), view_dir), 0.0, 1.0);
    float fresnel = pow(facing, 0.2);
    float alpha = clamp((1.0 - fresnel) + material_color.a, 0.0, 1.0);
    float reflectivity = reflection_strength * normal_sample.a;

    ALBEDO = diffuse_sample.rgb;
    SPECULAR = 1.0;
    METALLIC = clamp(reflectivity, 0.0, 1.0);
    ROUGHNESS = glossiness_to_roughness(abs(glossiness));
    NORMAL_MAP = decode_normal_from_rg(normal_sample);
    NORMAL_MAP_DEPTH = 1.0;
    ALPHA = alpha;
}
