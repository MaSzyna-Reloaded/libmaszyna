// types/sunlessnormalmap.gdshader cut out, both faces drawn, with the specgloss texture
shader_type spatial;
render_mode depth_draw_opaque, cull_disabled, diffuse_burley, specular_schlick_ggx;

#define MASZYNA_ALPHA_SCISSOR
#define MASZYNA_SPECGLOSS
#include "res://addons/libmaszyna/legacy/materials/types/sunlessnormalmap.gdshaderinc"
