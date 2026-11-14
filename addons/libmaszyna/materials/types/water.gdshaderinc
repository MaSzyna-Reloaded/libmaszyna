// The code of the water material type, for its shader (water.gdshader) and its variants
// (water_*.gdshader, MaszynaMaterialFactory.SHADER_VARIANTS), each of them with its own render_mode
// and defines; water_specgloss.gdshaderinc is the same water, its glossiness taken of its specgloss
// map too.
//
// The original's water (mat_water.frag) as it looks: its texture's colour, its normal map and its
// glossiness, the first two read where its dudv map moves them to, at the material's wave_speed and
// wave_strength - which are the material's at a light wind, and the environment's wind
// (reloaded_wind_speed) raises them as the wind raises the waves. What it reflects is water's
// instead of the original's: as much as the fresnel of water's index of refraction lets, little
// looking down, all of it at a grazing angle - its shore, found by a reflection of its own marched
// over the scene drawn before it (Godot gives a surface seen through no screen-space reflections),
// and the sky's radiance where that finds nothing. The rest is what lies under it, seen through the
// water as deep as it is and shifted by the waves, and the water's own colour (its texture's) where
// the water is too deep to see through. The original's alpha (its own "fresnel", pow(facing, 0.2))
// is not taken, nor its colour and reflection, which made that alpha. The sun mirrored in it is a
// disk as large as the sky's, and the rain rings it. Its roughness is at least that of the wind's
// waves a pixel cannot tell apart, so that far off the sun spreads over them in a path of glitter
// instead of a mirror's glare.

#include "res://addons/libreloaded/environment/ground_weather.gdshaderinc"

// Water's reflectance looking straight at it (its index of refraction, 1.33), and what Godot's
// SPECULAR gives it: F0 = SPECULAR_F0_SCALE * SPECULAR^2
const float WATER_F0 = 0.02;
const float SPECULAR_F0_SCALE = 0.16;
// The wind a material's wave_speed and wave_strength are its waves' at [m/s]: the environment's
// own (EnvironmentNode, wind_strength 0.3 of WIND_SPEED_MIN..WIND_SPEED_MAX)
const float WAVE_REFERENCE_WIND = 1.0;
// How the waves' speed grows with the wind: as the phase speed of a fetch-limited sea's peak,
// g / omega_peak, omega_peak falling as wind^(-1/3) (JONSWAP, Hasselmann et al., 1973)
const float WAVE_SPEED_WIND_POWER = 1.0 / 3.0;
// How the waves' slope grows with the wind: as its spread, its variance growing with the wind (Cox
// and Munk, 1954)
const float WAVE_STRENGTH_WIND_POWER = 0.5;
// The share of the dudv map's first read that shifts its second (mat_water.frag)
const float DUDV_FIRST_SHIFT = 0.1;
// The original's roughness of a glossiness of none, and the least and the most of its roughness
// (mat_water.frag, glossiness_to_roughness())
const float GLOSSINESS_NONE_ROUGHNESS = 0.6;
const float ROUGHNESS_MIN = 0.03;
const float ROUGHNESS_MAX = 1.0;
// The growth of the variance of the water's slope per m/s of the wind (Cox and Munk, 1954); their
// calm sea's 0.003 is the ocean's swell, which a lake has not got
const float COX_MUNK_VARIANCE_PER_WIND = 0.00512;
// The least wind the waves' length is worked out for [m/s]: JONSWAP's peak has none at no wind
const float WAVE_WIND_MIN = 0.1;
// The stretch of water the wind blows over [m]: a lake's - it sets how long its waves grow
const float WATER_FETCH = 1000.0;
// JONSWAP's peak of a fetch-limited sea: omega_peak = 22 * (g^2 / (wind * fetch))^(1/3)
// (Hasselmann et al., 1973), its wavelength 2 pi g / omega_peak^2
const float JONSWAP_PEAK = 22.0;
const float GRAVITY = 9.81;
// The shortest waves the roughness is worked out from, against the peak's length: the spectrum's
// next band, about a third of it
const float WAVE_SHORT_RATIO = 0.3;
// GGX's alpha of a surface whose slopes vary by sigma: alpha = sqrt(2) * sigma
const float GGX_ALPHA_PER_SLOPE = 1.41421356;
// The least GGX alpha the highlight is worked out for: a mirror's, whose distribution is a spike
const float GGX_ALPHA_MIN = 0.0001;
// Its own screen-space reflection, as Godot gives none to a surface seen through: the reflected
// ray marched over the scene drawn before it in steps growing from SSR_FIRST_STEP [m] by
// SSR_STEP_GROWTH (some 290 m in SSR_STEPS), the hit narrowed in SSR_REFINE_STEPS halvings, the
// scene's colour read from a mipmap SSR_SCREEN_LOD levels down (a quarter of the screen's
// resolution) and faded out within SSR_EDGE_FADE of the screen's edge
const int SSR_STEPS = 16;
const int SSR_REFINE_STEPS = 4;
const float SSR_FIRST_STEP = 0.25;
const float SSR_STEP_GROWTH = 1.6;
const float SSR_SCREEN_LOD = 2.0;
const float SSR_EDGE_FADE = 0.08;
// a hit is a point of the ray behind what is drawn by less than this many of its last steps
const float SSR_HIT_STEPS = 2.0;
// the nearest the ray may come to the camera [m]
const float SSR_NEAR = 0.05;
// The water seen through: as deep as this [m], 1/e of the light from what lies under it comes
// through; what does not is the water's own colour (its texture's), lit as a body of this albedo
const float WATER_CLEAR_DEPTH = 2.0;
const float WATER_BODY_ALBEDO = 0.2;
// how far the waves shift what is seen through the water [share of the screen], at a metre of it
const float REFRACTION_SHIFT = 0.03;
// A surface whose up is this close to the world's Z takes its axes from X instead (fragment())
const float UP_ALONG_Z = 0.99;
// The least n.v the highlight is divided by: a surface seen edge-on
const float N_DOT_V_MIN = 0.0001;

// The environment's wind (EnvironmentNode): its speed [m/s]
global uniform float reloaded_wind_speed;
// Whether the shore is mirrored by the water's own screen-space reflection (MaszynaMaterialManager,
// a graphics option): without it, the sky's reflection alone
global uniform bool maszyna_water_screen_reflections;
// The sun's angular radius as the sky draws it [rad] (the sky's environment): the sun mirrored in
// the water is as large as the sun in the sky
global uniform float reloaded_sun_angular_radius;

uniform sampler2D diffuse_texture : source_color, filter_linear_mipmap_anisotropic, repeat_enable;
uniform sampler2D normal_texture : filter_linear_mipmap_anisotropic, repeat_enable;
uniform sampler2D dudv_texture : filter_linear_mipmap_anisotropic, repeat_enable;
#ifdef WATER_SPECGLOSS
uniform sampler2D specgloss_texture : filter_linear_mipmap_anisotropic, repeat_enable;
#endif

// the scene drawn before the water, and its depth
uniform sampler2D screen_texture : hint_screen_texture, filter_linear_mipmap;
uniform sampler2D depth_texture : hint_depth_texture, filter_nearest;

uniform float glossiness = 10.0;
uniform float wave_strength : hint_range(0.0, 1.0) = 0.05;
uniform float wave_speed : hint_range(0.0, 1.0) = 0.02;

varying vec3 v_world_position;
varying vec3 v_world_normal;

// The original's roughness of its glossiness, the exponent of its highlight
float glossiness_to_roughness(float source_glossiness) {
    if (source_glossiness <= 0.0) {
        return GLOSSINESS_NONE_ROUGHNESS;
    }
    return clamp(sqrt(2.0 / (source_glossiness + 2.0)), ROUGHNESS_MIN, ROUGHNESS_MAX);
}

// The original's uv moved by its dudv map: read once, gone with the waves, and read again where the
// first read moved it, as far as the waves are strong
vec2 distort_uv(vec2 uv, float speed, float strength) {
    float move_factor = mod(TIME * speed, 1.0);
    vec2 first = texture(dudv_texture, vec2(uv.x + move_factor, uv.y)).rg * DUDV_FIRST_SHIFT;
    vec2 distortion = texture(dudv_texture, uv + vec2(first.x, first.y + move_factor)).rg * 2.0 - 1.0;
    return uv + distortion * strength;
}

// The point of the scene drawn at uv, in view space
vec3 scene_view_position(vec2 uv, mat4 inv_projection) {
	float depth = textureLod(depth_texture, uv, 0.0).r;
	vec4 position = inv_projection * vec4(uv * 2.0 - 1.0, depth, 1.0);
	return position.xyz / position.w;
}

// Where on the screen a point in view space is drawn
vec2 screen_uv_of(vec3 position, mat4 projection) {
	vec4 clip = projection * vec4(position, 1.0);
	return clip.xy / clip.w * 0.5 + 0.5;
}

// The ray from origin along direction (view space) marched over the scene: where it meets it on the
// screen (xy), how much of it to take (z: faded at the screen's edge; 0 - missed)
vec3 march_reflection(vec3 origin, vec3 direction, mat4 projection, mat4 inv_projection, float jitter) {
	float near_t = 0.0;
	float t = SSR_FIRST_STEP * (0.5 + jitter);
	for (int i = 0; i < SSR_STEPS; i++) {
		vec3 point = origin + direction * t;
		if (point.z > -SSR_NEAR) {
			break;
		}
		vec2 uv = screen_uv_of(point, projection);
		if (any(lessThan(uv, vec2(0.0))) || any(greaterThan(uv, vec2(1.0)))) {
			break;
		}
		float behind = scene_view_position(uv, inv_projection).z - point.z;
		if (behind > 0.0 && behind < (t - near_t) * SSR_HIT_STEPS) {
			float far_t = t;
			for (int k = 0; k < SSR_REFINE_STEPS; k++) {
				float middle = (near_t + far_t) * 0.5;
				vec3 halfway = origin + direction * middle;
				if (scene_view_position(screen_uv_of(halfway, projection), inv_projection).z > halfway.z) {
					far_t = middle;
				} else {
					near_t = middle;
				}
			}
			uv = screen_uv_of(origin + direction * far_t, projection);
			vec2 edge = min(uv, 1.0 - uv);
			return vec3(uv, smoothstep(0.0, SSR_EDGE_FADE, min(edge.x, edge.y)));
		}
		near_t = t;
		t *= SSR_STEP_GROWTH;
	}
	return vec3(0.0);
}

void vertex() {
    v_world_position = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
    v_world_normal = normalize(MODEL_NORMAL_MATRIX * NORMAL);
}

void fragment() {
    float wind_ratio = max(reloaded_wind_speed, 0.0) / WAVE_REFERENCE_WIND;
    vec2 water_uv = distort_uv(UV, wave_speed * pow(wind_ratio, WAVE_SPEED_WIND_POWER),
            wave_strength * pow(wind_ratio, WAVE_STRENGTH_WIND_POWER));

    // the normal map's normal on the surface's own tangents (as Godot's NORMAL_MAP)
    vec3 mapped = vec3(texture(normal_texture, water_uv).rg * 2.0 - 1.0, 0.0);
    mapped.z = sqrt(1.0 - clamp(dot(mapped.xy, mapped.xy), 0.0, 1.0));
    vec3 normal = normalize(TANGENT * mapped.x + BINORMAL * mapped.y + NORMAL * mapped.z);
    // and the rain's rings on it (the core's ground weather), their slopes along the world's axes
    // over the surface's own plane
    vec2 ground = v_world_position.xz;
    vec2 ripples = reloaded_weather_rain_ripples(ground, length(fwidth(ground)));
    vec3 up = normalize(v_world_normal);
    vec3 across = normalize(cross(up, abs(up.z) < UP_ALONG_Z ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0)));
    vec3 along = cross(across, up);
    NORMAL = normalize(normal + (VIEW_MATRIX * vec4(across * ripples.x + along * ripples.y, 0.0)).xyz);

    float source_glossiness = abs(glossiness);
#ifdef WATER_SPECGLOSS
    source_glossiness *= texture(specgloss_texture, UV).g;
#endif
    vec3 water_colour = texture(diffuse_texture, water_uv).rgb;
    METALLIC = 0.0;
    // the roughness is the material's glossiness, and at least the slopes of the wind's waves the
    // pixel cannot tell apart: none of the waves it is smaller than, all of them as it spans their
    // crests - near, the waves mirror the sun sharply; far, its light spreads over them in a path
    // of glitter, as wide as the wind makes them steep
    float wave_wind = max(reloaded_wind_speed, WAVE_WIND_MIN);
    float slope_sigma = sqrt(COX_MUNK_VARIANCE_PER_WIND * wave_wind);
    float peak_frequency = JONSWAP_PEAK * pow(GRAVITY * GRAVITY / (wave_wind * WATER_FETCH), 1.0 / 3.0);
    float peak_length = TAU * GRAVITY / (peak_frequency * peak_frequency);
    float pixel_size = max(length(dFdx(v_world_position)), length(dFdy(v_world_position)));
    float unresolved = clamp(pixel_size / (peak_length * WAVE_SHORT_RATIO), 0.0, 1.0);
    // Godot's roughness is the square root of GGX's alpha
    ROUGHNESS = max(glossiness_to_roughness(source_glossiness),
            sqrt(GGX_ALPHA_PER_SLOPE * slope_sigma * sqrt(unresolved)));

    float n_dot_v = clamp(dot(NORMAL, VIEW), 0.0, 1.0);
    float fresnel = WATER_F0 + (1.0 - WATER_F0) * pow(1.0 - n_dot_v, 5.0);
    // the shore it mirrors, marched over the screen; the sky's reflection (the environment's)
    // where the ray finds nothing - SPECULAR's F0 scales both of Godot's terms of it
    float jitter = fract(52.9829189 * fract(dot(FRAGCOORD.xy, vec2(0.06711056, 0.00583715))));
    vec3 hit = vec3(0.0);
    if (maszyna_water_screen_reflections) {
        hit = march_reflection(VERTEX, reflect(normalize(VERTEX), NORMAL), PROJECTION_MATRIX,
                INV_PROJECTION_MATRIX, jitter);
    }
    vec3 mirrored = textureLod(screen_texture, hit.xy, SSR_SCREEN_LOD).rgb;
    SPECULAR = sqrt(WATER_F0 / SPECULAR_F0_SCALE * (1.0 - hit.z));
    // what lies under it, shifted by the waves and dimmed by the water it is seen through
    vec3 bottom = scene_view_position(SCREEN_UV, INV_PROJECTION_MATRIX);
    float through = max(length(bottom) - length(VERTEX), 0.0);
    vec2 refracted_uv = SCREEN_UV + NORMAL.xy * REFRACTION_SHIFT * min(through, 1.0);
    if (scene_view_position(refracted_uv, INV_PROJECTION_MATRIX).z > VERTEX.z) {
        refracted_uv = SCREEN_UV;
    }
    float transmitted = exp(-through / WATER_CLEAR_DEPTH);
    EMISSION = textureLod(screen_texture, refracted_uv, 0.0).rgb * transmitted * (1.0 - fresnel)
            + mirrored * fresnel * hit.z;
    ALBEDO = water_colour * WATER_BODY_ALBEDO * (1.0 - transmitted) * (1.0 - fresnel);
}

// GGX's distribution of the microfacets of alpha^2 (alpha2) at n_dot_h
float ggx_distribution(float n_dot_h, float alpha2) {
    float denominator = n_dot_h * n_dot_h * (alpha2 - 1.0) + 1.0;
    return alpha2 / (PI * denominator * denominator);
}

// The diffuse light and the highlight of every light, the sun's too: GGX of the glossiness's
// roughness, Schlick's fresnel of water, Smith's visibility. SPECULAR_AMOUNT is the light's own
// specular share, as the game dims the sun's in a fog that hides it.
void light() {
    float n_dot_l = clamp(dot(NORMAL, LIGHT), 0.0, 1.0);
    DIFFUSE_LIGHT += n_dot_l * ATTENUATION * LIGHT_COLOR / PI;
    float alpha = max(ROUGHNESS * ROUGHNESS, GGX_ALPHA_MIN);
    vec3 light_direction = LIGHT;
    float normalisation = 1.0;
    if (LIGHT_IS_DIRECTIONAL) {
        // the sun is a disk: its highlight is the point of it nearest the mirrored view (Karis's
        // representative point, "Real Shading in Unreal Engine 4", 2013), the lobe's energy kept
        // by (alpha / (alpha + radius / 2))^2 - a mirror shows the whole disk, not a spike
        vec3 mirrored = reflect(-VIEW, NORMAL);
        vec3 to_ray = dot(LIGHT, mirrored) * mirrored - LIGHT;
        float radius = tan(reloaded_sun_angular_radius);
        light_direction = normalize(LIGHT + to_ray * clamp(radius / max(length(to_ray), GGX_ALPHA_MIN), 0.0, 1.0));
        float widened = clamp(alpha + radius / 2.0, 0.0, 1.0);
        normalisation = (alpha / widened) * (alpha / widened);
    }
    vec3 half_vector = normalize(light_direction + VIEW);
    float n_dot_v = clamp(dot(NORMAL, VIEW), N_DOT_V_MIN, 1.0);
    float n_dot_h = clamp(dot(NORMAL, half_vector), 0.0, 1.0);
    float v_dot_h = clamp(dot(VIEW, half_vector), 0.0, 1.0);
    float fresnel = WATER_F0 + (1.0 - WATER_F0) * pow(1.0 - v_dot_h, 5.0);
    // Schlick-GGX's k, and Smith's visibility: G / (4 n.l n.v)
    float k = alpha / 2.0;
    float visibility = 1.0 / (4.0 * (n_dot_l * (1.0 - k) + k) * (n_dot_v * (1.0 - k) + k));
    // n_dot_l is 0 for a light behind the surface: no highlight
    SPECULAR_LIGHT += ggx_distribution(n_dot_h, alpha * alpha) * normalisation * visibility * fresnel * n_dot_l
            * ATTENUATION * LIGHT_COLOR * SPECULAR_AMOUNT;
}
