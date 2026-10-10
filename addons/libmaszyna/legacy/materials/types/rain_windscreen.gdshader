// Windshield rain effects, ported from the original mat_rain_windscreen.frag + random.glsl
// (by @lcddisplay; wiper movement @MichauSto).

shader_type spatial;
render_mode blend_mix, cull_back, diffuse_burley, specular_schlick_ggx;

uniform sampler2D diffuse_texture : source_color, filter_linear_mipmap, repeat_enable;
// 2x2 atlas of droplet sprites
uniform sampler2D raindrops_atlas : source_color, filter_linear_mipmap, repeat_enable;
// one wiper per channel: the phase of its sweep at which the blade passes the texel, 0 outside of its area.
// sRGB like in the original ("#texture(wipermask, 2, sRGB_A)"): only decoded it is linear in the
// angle of the arm - read raw, the wiped edge runs far away from the blade.
uniform sampler2D wiper_mask : source_color, hint_default_black, filter_linear, repeat_enable;

// stands in for the "ambient" of the original, see fragment()
uniform sampler2D screen_texture : hint_screen_texture, filter_linear_mipmap, repeat_disable;

uniform float raindrop_grid_size = 1.0;
uniform float specular : hint_range(0.0, 1.0, 0.01) = 0.5;
uniform float roughness : hint_range(0.0, 1.0) = 0.2;

// A droplet takes a few pixels of the screen and a 512 px quadrant of the atlas: the mip level
// picked for it averages the rim away and its alpha falls below the 0.3 of sharp_alpha.
uniform float drop_lod_bias = -2.0;
uniform float specular_intensity = 1.0;

// Beyond the original, where the heaviest rain still looks like a drizzle: droplets bend the view
// behind them, a second layer of large droplets joins from heavy_rain_start, and a film of running
// water blurs the whole glass. All of it grows with the rain and is cleared by the wipers.
uniform float refraction_strength = 0.04;
uniform float heavy_rain_start : hint_range(0.0, 1.0) = 0.4;
uniform float large_drop_scale = 2.0;
uniform float film_strength : hint_range(0.0, 1.0) = 0.9;
uniform float film_blur = 6.0;
uniform float film_distortion = 0.03;
// rivulets: columns of the droplet grid one of them takes, the share of a column its width takes,
// its length and speed in UV of the glass
uniform float rivulet_spacing = 6.0;
uniform float rivulet_width : hint_range(0.0, 0.5) = 0.11;
uniform float rivulet_length = 0.35;
uniform float rivulet_speed = 0.25;
// water the blade pushes to the ends of its sweep, where it slows down and turns: the share of
// the sweep the two bands take and the seconds they keep running after the wipers stopped
uniform float edge_rivulet_band : hint_range(0.0, 0.5) = 0.12;
uniform float edge_rivulet_fade = 6.0;
// the rain the blade starts to gather visible water from - the bead, the ridges at the ends of
// the sweep and the rivulets draining them grow from here up to the heaviest rain
uniform float gathered_water_start : hint_range(0.0, 1.0) = 0.6;
// the bead of water the blade pushes ahead of itself, in the share of the sweep
uniform float blade_bead_width : hint_range(0.0, 0.2) = 0.05;
uniform float wobble_strength = 0.002;
uniform float wobble_speed = 30.0;

// rain_params.x / rain_params.y, wiper_pos, wiper_timer_out, wiper_timer_return of the original
// scene uniforms (opengl33renderer.cpp:750-790)
global uniform float maszyna_rain_intensity;
global uniform float maszyna_wiper_regen_time;
// per wiper: 0..1 sweeping out, 1..2 returning
global uniform vec4 maszyna_wiper_pos;
// per wiper: seconds since it last left the parked position and the far end (the original passes
// the two as points in time, "time - wiper_timer_*")
global uniform vec4 maszyna_wiper_since_out;
global uniform vec4 maszyna_wiper_since_return;

// Evolving Sub-Grid Turbulence for Smoke Animation, H. Schechter and R. Bridson (random.glsl)
uint hash(uint s) {
    s ^= 2747636419u;
    s *= 2654435769u;
    s ^= s >> 16u;
    s *= 2654435769u;
    s ^= s >> 16u;
    s *= 2654435769u;
    return s;
}

uint hash_combine(uint seed, uint value) {
    return seed ^ (hash(value) + 0x9e3779b9u + (seed << 6u) + (seed >> 2u));
}

uint hash_cell(ivec2 cell, uint side) {
    return hash_combine(hash_combine(hash(uint(cell.x)), uint(cell.y)), side);
}

float rand_f(inout uint seed) {
    seed = hash(seed);
    return float(seed) * 2.3283064365386963e-10;
}

// Sampled with explicit gradients: the droplets are drawn inside of loops and branches, where
// the implicit derivatives of texture() are undefined - the mip level (and with it the look of
// the droplets) would depend on where on the screen and how far the glass is.
vec4 get_drop_tex(float choice, vec2 uv, vec2 uv_dx, vec2 uv_dy) {
    vec2 offset;
    if (choice < 0.25) {
        offset = vec2(0.0, 0.0);
    } else if (choice < 0.5) {
        offset = vec2(0.5, 0.0);
    } else if (choice < 0.75) {
        offset = vec2(0.0, 0.5);
    } else {
        offset = vec2(0.5, 0.5);
    }
    float lod_scale = 0.5 * exp2(drop_lod_bias);
    return textureGrad(raindrops_atlas, offset + uv * 0.5, uv_dx * lod_scale, uv_dy * lod_scale);
}

// How much of the rain is left at the given point after the wipers; side tells the two halves of
// a wiper's area apart, so that each regrows its own droplets.
float get_mix_factor(vec2 co, out float side) {
    vec4 move_phase = maszyna_wiper_pos;
    vec4 is_out = vec4(lessThanEqual(move_phase, vec4(1.0)));
    move_phase = mix(vec4(2.0) - move_phase, move_phase, is_out);

    // asked for per cell centre and per fragment alike: no mip levels
    vec4 mask = textureLod(wiper_mask, co, 0.0);
    vec4 area_mask = step(0.001, mask);

    vec4 mask_val = mix(1.0 - mask, mask, is_out);
    vec4 wipe_width = smoothstep(1.0, 0.9, move_phase) * 0.25;
    vec4 cleaned = smoothstep(move_phase - wipe_width, move_phase, mask_val) * area_mask;
    vec4 side_v = step(mask_val, move_phase);
    cleaned *= side_v;
    side_v = mix(side_v, vec4(1.0) - side_v, is_out);

    // "regeneration", raindrops gradually returning after wiper pass
    vec4 regen_phase = clamp(
            (mix(maszyna_wiper_since_out, maszyna_wiper_since_return, side_v) - vec4(0.2))
                    / vec4(maszyna_wiper_regen_time),
            vec4(0.0), vec4(1.0));

    side_v = mix(vec4(0.0), vec4(1.0) - side_v, area_mask);

    vec4 factor_v = mix(vec4(1.0), regen_phase * (vec4(1.0) - cleaned), area_mask);

    side = 0.0;
    float out_factor = 1.0;

    // Find out the wiper blade that influences given grid cell the most. The original takes a
    // wiper only while its factor is below 1: the moment the droplets of a cell have fully
    // returned its side falls back to 0, the seed of the cell changes and every droplet of the
    // wiped area is dealt anew at once. The first wiper the cell belongs to is taken here even then.
    bool found = false;
    for (int i = 0; i < 4; ++i) {
        if (area_mask[i] > 0.0 && (!found || factor_v[i] < out_factor)) {
            out_factor = factor_v[i];
            side = side_v[i];
            found = true;
        }
    }

    return out_factor;
}

// One layer of droplets (the loop of the original main()), accumulated into the out parameters.
void add_droplets(
        vec2 uv, vec2 uv_dx, vec2 uv_dy, float grid_size, float activation, float sun_factor,
        inout vec3 drop_layer, inout float drop_mask_sum, inout vec2 lens_offset, inout float lens_sum) {
    const float cycle_duration = 4.0;

    float square_min = 0.5 / grid_size;
    float square_max = 1.2 / grid_size;

    vec2 cell = floor(uv * grid_size);

    // Grid of 9 droplets in immediate neighbourhood
    for (int oy = -1; oy <= 1; oy++) {
        for (int ox = -1; ox <= 1; ox++) {
            vec2 neighbor_cell = cell + vec2(float(ox), float(oy));
            vec2 neighbor_center = (neighbor_cell + 0.5) / grid_size;

            float side;
            float mix_factor = get_mix_factor(neighbor_center, side);

            uint seed = hash_cell(ivec2(neighbor_cell), uint(side));

            float returned = mix_factor - rand_f(seed);
            if (returned < 0.0) {
                continue;
            }
            // the original switches a returning droplet on at once, here it grows back
            float appear = smoothstep(0.0, 0.15, returned);

            // Show a percentage of droplets given by rain intensity
            if (rand_f(seed) > activation) {
                continue;
            }

            // Randomly modulate droplet center & size
            vec2 drop_center = (neighbor_cell + vec2(rand_f(seed), rand_f(seed))) / grid_size;
            float square_size = mix(square_min, square_max, rand_f(seed));

            float life_time = TIME + rand_f(seed) * cycle_duration;
            float phase = fract(life_time / cycle_duration);
            float active = clamp(1.0 - phase, 0.0, 1.0);

            // Gravity influence
            float gravity_phase = smoothstep(0.5, 1.0, phase);
            float drop_mass = mix(0.3, 1.2, rand_f(seed));
            float gravity_speed = 0.15 * drop_mass;
            vec2 gravity_offset = vec2(0.0, gravity_phase * gravity_speed * phase);

            // Random wobble
            bool has_wobble = rand_f(seed) < 0.10;
            vec2 wobble_offset = vec2(0.0);
            if (has_wobble && gravity_phase > 0.0) {
                float intensity = sin(TIME * wobble_speed + rand_f(seed) * 100.0) * wobble_strength * gravity_phase;
                wobble_offset = vec2(intensity, 0.0);
            }

            vec2 slide_offset = gravity_offset + wobble_offset;

            // Flatten droplets influenced by gravity
            float flatten_amount = smoothstep(0.1, 0.5, gravity_phase);
            float flatten_x = mix(1.0, 0.4, flatten_amount);
            float stretch_y = mix(1.0, 1.6, flatten_amount);

            // Droplet local position & mask
            vec2 diff = (uv + slide_offset) - drop_center;
            diff.x *= 1.0 / flatten_x;
            diff.y *= 1.0 / stretch_y;
            float mask = smoothstep(square_size * 0.5, square_size * 0.45, max(abs(diff.x), abs(diff.y)));

            if (mask > 0.001) {
                vec2 local_uv = (diff + square_size * 0.5) / square_size;
                float choice = rand_f(seed);
                vec4 drop_tex = get_drop_tex(choice, local_uv, uv_dx / square_size, uv_dy / square_size);
                float sharp_alpha = smoothstep(0.3, 0.9, drop_tex.a);

                float color_luma = length(drop_tex.rgb);
                float alpha_range = smoothstep(0.1, 0.3, color_luma);
                float black_alpha = mix(0.25, 0.85, alpha_range);

                float sparkle = 0.0;
                if (has_wobble) {
                    sparkle = pow(abs(sin(uv.x * 8.0 + TIME * 2.0 + rand_f(seed) * 6.2831)), 40.0)
                            * mix(0.2, 1.0, sun_factor);
                }

                vec3 drop_lit = drop_tex.rgb * sun_factor + vec3(specular_intensity * sparkle) * sharp_alpha;

                mask *= appear;
                drop_layer += drop_lit * sharp_alpha * active * black_alpha * mask;
                drop_mask_sum += sharp_alpha * active * black_alpha * mask;
                // the droplet as a lens: the view behind it is mirrored around its centre
                lens_offset -= diff / square_size * sharp_alpha * active * mask;
                lens_sum += sharp_alpha * active * mask;
            }
        }
    }
}

// Value noise of the water film
float film_noise(vec2 p) {
    ivec2 i = ivec2(floor(p));
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    uint s00 = hash_cell(i, 7u);
    uint s10 = hash_cell(i + ivec2(1, 0), 7u);
    uint s01 = hash_cell(i + ivec2(0, 1), 7u);
    uint s11 = hash_cell(i + ivec2(1, 1), 7u);
    return mix(mix(rand_f(s00), rand_f(s10), f.x), mix(rand_f(s01), rand_f(s11), f.x), f.y);
}

// Water running down the glass in wandering streams, each with a head and a fading trail. "Down"
// is towards the smaller v, as for the droplets pulled by the gravity.
void add_rivulets(
        vec2 uv, float columns, float width, float activation, float sun_factor,
        inout vec3 drop_layer, inout float drop_mask_sum, inout vec2 lens_offset, inout float lens_sum) {
    float column = floor(uv.x * columns);
    for (int ox = -1; ox <= 1; ox++) {
        float neighbor_column = column + float(ox);
        uint seed = hash_cell(ivec2(int(neighbor_column), 0), 11u);
        float returned = activation - rand_f(seed);
        if (returned < 0.0) {
            continue;
        }
        float speed = rivulet_speed * mix(0.5, 1.5, rand_f(seed));
        float travel = 1.0 + rivulet_length;
        float head = 1.0 - fract(TIME * speed / travel + rand_f(seed)) * travel;
        float behind = uv.y - head;
        if (behind < 0.0 || behind > rivulet_length) {
            continue;
        }
        float wander = film_noise(vec2(neighbor_column * 17.0, uv.y * columns * 0.6)) - 0.5;
        float center = (neighbor_column + 0.5 + wander * 0.9) / columns;
        float offset = (uv.x - center) / width;
        // wider at the head, thinning out along the trail
        float fade = 1.0 - behind / rivulet_length;
        float body = smoothstep(1.0, 0.6, abs(offset) / mix(0.5, 1.0, fade)) * smoothstep(0.0, 0.1, fade)
                * smoothstep(0.0, 0.15, returned);
        float edge = body * smoothstep(0.3, 0.8, abs(offset));

        // a lit edge and a shaded one, like the rim of the droplets - the bent view alone does not
        // show over a plain background
        drop_layer += vec3(sun_factor) * edge * step(0.0, offset) * 0.7;
        drop_mask_sum += edge * mix(0.45, 0.7, step(0.0, offset));
        lens_offset -= vec2(offset, 0.0) * body;
        lens_sum += body;
    }
}

// How much water runs down at the given point of the ends of a wiper's sweep: the mask is the
// phase of the sweep, so its lowest and highest values are the two ends.
float get_sweep_end(vec2 uv) {
    vec4 mask = textureLod(wiper_mask, uv, 0.0);
    vec4 band = smoothstep(edge_rivulet_band, 0.0, mask) + smoothstep(1.0 - edge_rivulet_band, 1.0, mask);
    // a wiper at work was at its far end a moment ago
    vec4 working = vec4(1.0) - smoothstep(vec4(edge_rivulet_fade * 0.3), vec4(edge_rivulet_fade), maszyna_wiper_since_return);
    vec4 ends = step(0.001, mask) * band * working;
    return max(max(ends.r, ends.g), max(ends.b, ends.a));
}

// The water the blade gathers: a bead pushed just ahead of it in the phase of the sweep (see
// get_mix_factor() for the phase), growing on its way and uneven along the blade; left behind
// at the two ends of the sweep it piles up there as a ridge that drains away (the rivulets of
// get_sweep_end() carry it down). bead_side is -1 at the inner side and 1 at the outer rim,
// bead_direction the way across it on the screen.
float get_blade_bead(vec2 uv, float grid_size, out float bead_side, out vec2 bead_direction) {
    vec4 move_phase = maszyna_wiper_pos;
    vec4 is_out = vec4(lessThanEqual(move_phase, vec4(1.0)));
    move_phase = mix(vec4(2.0) - move_phase, move_phase, is_out);
    vec4 mask = textureLod(wiper_mask, uv, 0.0);
    // taken here, outside of any branch
    vec4 mask_dx = dFdx(mask);
    vec4 mask_dy = dFdy(mask);
    vec4 area = step(0.001, mask);
    vec4 mask_val = mix(1.0 - mask, mask, is_out);

    float uneven = mix(0.6, 1.4, film_noise(uv * grid_size * 0.15 + vec2(0.0, TIME * 0.5)));
    float gathered = smoothstep(gathered_water_start, 1.0, maszyna_rain_intensity);
    float width = blade_bead_width * uneven * mix(0.3, 1.0, gathered);

    // pushed by the moving blade
    vec4 moving = area * step(0.001, move_phase) * step(move_phase, vec4(0.999));
    vec4 across_moving = (mask_val - move_phase) / (width * mix(vec4(0.4), vec4(1.0), move_phase));
    // piled up at the far end since the blade turned there, at the parked end while it rests
    vec4 draining = exp(-maszyna_wiper_since_return / vec4(edge_rivulet_fade * 0.5));
    vec4 across_far = (vec4(1.0) - mask) / (width * draining);
    vec4 parked = is_out * step(maszyna_wiper_pos, vec4(0.025));
    vec4 across_parked = mask / (width * draining);

    float body = 0.0;
    bead_side = 0.0;
    bead_direction = vec2(0.0);
    for (int i = 0; i < 4; ++i) {
        // x: across the bead, y: its strength, z: which way the mask grows across it
        vec3 beads[3] = {
            vec3(across_moving[i], moving[i], is_out[i] * 2.0 - 1.0),
            vec3(across_far[i], area[i] * draining[i], -1.0),
            vec3(across_parked[i], area[i] * parked[i] * draining[i], 1.0)
        };
        for (int k = 0; k < 3; ++k) {
            float candidate = beads[k].y * step(0.0, beads[k].x) * smoothstep(1.0, 0.5, beads[k].x);
            if (candidate > body) {
                body = candidate;
                bead_side = beads[k].x * 2.0 - 1.0;
                vec2 gradient = vec2(mask_dx[i], mask_dy[i]) * beads[k].z;
                bead_direction = gradient / max(length(gradient), 0.00001);
            }
        }
    }
    return body * gathered;
}

void fragment() {
    vec4 tex_color = texture(diffuse_texture, UV);
    if (tex_color.a < 0.01) {
        discard;
    }

    float grid_size = ceil(raindrop_grid_size);

    // The droplets of the original are not lit: their colour (a white rim in the atlas) is scaled
    // by the luminance of the ambient light only. A fragment shader has no ambient here, and lit
    // through ALBEDO the rims go black inside of a dark cab - so the droplets are emitted, scaled
    // by the blurred luminance of what is seen through the glass.
    float amb_lum = clamp(dot(textureLod(screen_texture, SCREEN_UV, 6.0).rgb, vec3(0.2126, 0.7152, 0.0722)), 0.0, 1.0);
    float sun_factor = pow(clamp(amb_lum * 6.0, 0.0, 1.0), 0.5);

    vec3 drop_layer = vec3(0.0);
    float drop_mask_sum = 0.0;
    vec2 lens_offset = vec2(0.0);
    float lens_sum = 0.0;
    vec2 uv_dx = dFdx(UV);
    vec2 uv_dy = dFdy(UV);
    add_droplets(
            UV, uv_dx, uv_dy, grid_size, maszyna_rain_intensity, sun_factor,
            drop_layer, drop_mask_sum, lens_offset, lens_sum);

    float heavy_rain = smoothstep(heavy_rain_start, 1.0, maszyna_rain_intensity);
    if (heavy_rain > 0.0) {
        add_droplets(
                UV, uv_dx, uv_dy, max(1.0, ceil(grid_size / large_drop_scale)), heavy_rain, sun_factor,
                drop_layer, drop_mask_sum, lens_offset, lens_sum);
    }

    // Rivulets and the film of water running down the glass, wiped like the droplets
    float wiped_side;
    float wiped = get_mix_factor(UV, wiped_side);
    float rivulet_columns = max(1.0, ceil(grid_size / rivulet_spacing));
    float rivulet_column_width = rivulet_width / rivulet_columns;
    add_rivulets(
            UV, rivulet_columns, rivulet_column_width, heavy_rain * wiped, sun_factor,
            drop_layer, drop_mask_sum, lens_offset, lens_sum);

    // three times as dense at the ends of the sweep and not wiped away: the water the blade has
    // piled up there, draining
    float sweep_end = get_sweep_end(UV) * smoothstep(gathered_water_start, 1.0, maszyna_rain_intensity);
    if (sweep_end > 0.0) {
        add_rivulets(
                UV, rivulet_columns * 3.0, rivulet_column_width, clamp(sweep_end * 2.0, 0.0, 1.0), sun_factor,
                drop_layer, drop_mask_sum, lens_offset, lens_sum);
    }

    float bead_side;
    vec2 bead_direction;
    float bead = get_blade_bead(UV, grid_size, bead_side, bead_direction);
    // lit far rim, shaded at the blade - like the rivulets
    float bead_rim = bead * smoothstep(0.2, 0.8, bead_side);
    drop_layer += vec3(sun_factor) * bead_rim;
    drop_mask_sum += bead_rim + bead * smoothstep(-0.2, -0.8, bead_side) * 0.6;
    lens_offset -= bead_direction * bead_side * bead * 2.0;
    lens_sum += bead;

    float film = heavy_rain * film_strength * wiped;
    vec2 film_uv = UV * grid_size;
    float streaks = film_noise(vec2(film_uv.x * 0.35, film_uv.y * 0.03 + TIME * 0.6));
    float ripples = film_noise(vec2(film_uv.x * 0.1, film_uv.y * 0.1 + TIME * 0.25));
    vec2 film_offset = vec2(streaks - 0.5, (ripples - 0.5) * 0.5) * film_distortion * film;
    vec3 film_color = textureLod(screen_texture, SCREEN_UV + film_offset, film * film_blur).rgb;

    float lens_alpha = clamp(lens_sum, 0.0, 1.0);
    vec3 lens_color = textureLod(screen_texture, SCREEN_UV + film_offset + lens_offset * refraction_strength, 1.0).rgb;

    // film, lenses, then the droplets of the original ("drop_layer" blended by "drop_mask_sum") -
    // composited premultiplied, front over back
    float drop_alpha = clamp(drop_mask_sum, 0.0, 1.0);
    vec3 water_color = film_color * film;
    float water_alpha = film;
    water_color = lens_color * lens_alpha + water_color * (1.0 - lens_alpha);
    water_alpha = lens_alpha + water_alpha * (1.0 - lens_alpha);
    water_color = drop_layer * drop_alpha + water_color * (1.0 - drop_alpha);
    water_alpha = drop_alpha + water_alpha * (1.0 - drop_alpha);

    // Overlay windshield texture with alpha
    // glass over water over the view: blend_mix multiplies by ALPHA, so both are divided by it
    ALPHA = mix(water_alpha, 1.0, tex_color.a);
    ALBEDO = tex_color.rgb * tex_color.a / max(ALPHA, 0.001);
    EMISSION = water_color * (1.0 - tex_color.a) / max(ALPHA, 0.001);
    SPECULAR = specular;
    ROUGHNESS = roughness;
    METALLIC = 0.0;
}
