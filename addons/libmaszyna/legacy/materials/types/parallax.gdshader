shader_type spatial;

render_mode cull_back, depth_draw_opaque, diffuse_burley, specular_schlick_ggx;

uniform sampler2D diffuse_texture : source_color, filter_linear_mipmap_anisotropic, repeat_enable;
uniform sampler2D normal_texture : filter_linear_mipmap_anisotropic, repeat_enable;
// detail_parallax: a tiled detail normal map added to the normal map (mat_detail_parallax.frag)
uniform sampler2D detail_normal_texture : filter_linear_mipmap_anisotropic, repeat_enable;
uniform bool use_detail_normal = false;
uniform float detail_scale = 1.0;
uniform float detail_height_scale = 1.0;

uniform vec4 albedo_multiplier : source_color = vec4(1.0);

uniform float specular_strength : hint_range(0.0, 1.0) = 0.05;
uniform float reflection_strength : hint_range(0.0, 1.0) = 0.0;
uniform float glossiness = 10.0;

uniform float height_scale : hint_range(0.0, 0.05) = 0.01;
uniform float height_offset : hint_range(-1.0, 1.0) = 0.05;

uniform int transparency = 0;
uniform bool use_alpha_scissor = false;
uniform float alpha_scissor_threshold : hint_range(0.0, 1.0) = 0.5;

const float height_strength = 0.60;  // picked by trial & error for taste
const float parallax_max_distance = 10.0;

const bool debug_show_height = false;
const bool debug_disable_parallax = false;

varying vec3 v_tangent_view;
varying vec3 v_binormal_view;
varying vec3 v_normal_view;
varying vec3 v_position_view;
varying float v_dist;

void vertex() {
    v_tangent_view = normalize(MODELVIEW_NORMAL_MATRIX * TANGENT);
    v_normal_view = normalize(MODELVIEW_NORMAL_MATRIX * NORMAL);
    v_binormal_view = normalize(MODELVIEW_NORMAL_MATRIX * -BINORMAL);

    v_position_view = (MODELVIEW_MATRIX * vec4(VERTEX, 1.0)).xyz;
    v_dist = length(v_position_view);
}

float read_height(vec2 uv) {
    // Kept raw for vendor parity; deep parallax in the original shader does not apply height_offset here.
    return texture(normal_texture, uv).b;
}

vec2 apply_parallax_mapping(vec2 uv, vec3 view_dir_tangent, float fragment_distance) {
    if (debug_disable_parallax || height_scale <= 0.0) {
        return uv;
    }

    if (fragment_distance > parallax_max_distance) {
        return uv;
    }

    vec3 view_dir = normalize(view_dir_tangent);

    const float min_layers = 8.0;
    const float max_layers = 32.0;

    float layers_weight = fragment_distance / 20.0;
    layers_weight = min(abs(view_dir.z), layers_weight);

    float num_layers = mix(max_layers, min_layers, clamp(layers_weight, 0.0, 1.0));

    float layer_depth = 1.0 / num_layers;
    float current_layer_depth = 0.0;

    vec2 p = view_dir.xy * (height_scale * height_strength);
    vec2 delta_uv = p / num_layers;

    vec2 current_uv = uv;
    float current_depth_value = read_height(current_uv);

    for (int i = 0; i < 32; i++) {
        if (i >= int(num_layers)) {
            break;
        }

        if (current_layer_depth >= current_depth_value) {
            break;
        }

        current_uv -= delta_uv;
        current_depth_value = read_height(current_uv);
        current_layer_depth += layer_depth;
    }

    vec2 prev_uv = current_uv + delta_uv;

    float after_depth = current_depth_value - current_layer_depth;
    float before_depth = read_height(prev_uv) - current_layer_depth + layer_depth;

    float denom = after_depth - before_depth;
    float weight = 0.0;

    if (abs(denom) > 0.0001) {
        weight = after_depth / denom;
    }

    weight = clamp(weight, 0.0, 1.0);

    return prev_uv * weight + current_uv * (1.0 - weight);
}

vec3 decode_normal_from_rg(vec4 normal_sample) {
    vec3 n;

    n.xy = normal_sample.rg * 2.0 - 1.0;
    n.z = sqrt(1.0 - clamp(dot(n.xy, n.xy), 0.0, 1.0));

    return n * 0.5 + 0.5;
}

void fragment() {
    vec3 T = normalize(v_tangent_view);
    vec3 B = normalize(v_binormal_view);
    vec3 N = normalize(v_normal_view);

    mat3 tbn = mat3(T, B, N);

    vec3 view_dir_view = normalize(-v_position_view);
    vec3 view_dir_tangent = normalize(transpose(tbn) * view_dir_view);

    vec2 parallax_uv = apply_parallax_mapping(UV, view_dir_tangent, v_dist);

    vec4 diffuse_sample = texture(diffuse_texture, parallax_uv) * albedo_multiplier;
    vec4 normal_sample = texture(normal_texture, parallax_uv);

#ifdef MASZYNA_ALPHA_BLEND
    ALPHA = diffuse_sample.a;
#endif
    if ((use_alpha_scissor || transparency == 2) && diffuse_sample.a < alpha_scissor_threshold) {
        discard;
    }

    if (debug_show_height) {
        float h = read_height(parallax_uv);

        ALBEDO = vec3(h);
        SPECULAR = 0.0;
        METALLIC = 0.0;
        ROUGHNESS = 1.0;
    } else {
        float source_specularity = (diffuse_sample.r + diffuse_sample.g + diffuse_sample.b) * 0.5;
        float source_reflection = reflection_strength * normal_sample.a;

        ALBEDO = diffuse_sample.rgb;

        SPECULAR = clamp(max(source_specularity, source_reflection), 0.0, 1.0);
        METALLIC = 0.0;

        vec3 normal = decode_normal_from_rg(normal_sample) * 2.0 - 1.0;
        if (use_detail_normal) {
            // mat_detail_parallax.frag: normal.xy + detail.xy * detail_height_scale, detail tiled by detail_scale
            vec2 detail_xy = (texture(detail_normal_texture, parallax_uv * detail_scale).rg * 2.0 - 1.0)
                    * detail_height_scale;
            normal = normalize(vec3(normal.xy + detail_xy, normal.z));
        }
        NORMAL_MAP = normal * 0.5 + 0.5;
        NORMAL_MAP_DEPTH = 1.0;

    }
}
