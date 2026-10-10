shader_type spatial;

instance uniform vec3 albedo_color = vec3(1.0, 1.0, 1.0);

void fragment() {
    ALBEDO = albedo_color;
}
