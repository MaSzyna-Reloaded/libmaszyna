// The original's glossiness, the Blinn-Phong exponent (light_common.glsl), as the roughness Godot
// lights with

// The same curve as MaszynaMaterialFactory._roughness_from_glossiness()
float glossiness_to_roughness(float source_glossiness) {
    float g = abs(source_glossiness);

    if(g < 0.0001) {
        return 1.0;
    }

    float max_glossiness = 3500.0;
    float min_roughness = 0.18;
    float max_roughness = 0.75;

    float t = log(g + 1.0) / log(max_glossiness + 1.0);

    return mix(max_roughness, min_roughness, clamp(t, 0.0, 1.0));
}
