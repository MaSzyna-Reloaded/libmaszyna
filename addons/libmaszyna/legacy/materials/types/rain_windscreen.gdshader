// Windshield rain effects, ported from the original mat_rain_windscreen.frag + random.glsl
// (by @lcddisplay; wiper movement @MichauSto).

shader_type spatial;
render_mode blend_mix, cull_back, diffuse_burley, specular_schlick_ggx;

#include "res://addons/libmaszyna/legacy/materials/types/rain_windscreen.gdshaderinc"
