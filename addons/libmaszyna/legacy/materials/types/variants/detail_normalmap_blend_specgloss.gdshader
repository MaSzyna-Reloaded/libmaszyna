// types/detail_normalmap.gdshader blended, with the specgloss texture
shader_type spatial;
render_mode depth_draw_opaque, cull_back, diffuse_burley, specular_schlick_ggx;

#define MASZYNA_ALPHA_BLEND
#define MASZYNA_SPECGLOSS
#include "res://addons/libmaszyna/legacy/materials/types/detail_normalmap.gdshaderinc"
