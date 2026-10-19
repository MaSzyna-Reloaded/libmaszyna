// normalmap.gdshader + a tiled detail normal map, ported from the original mat_detail_normalmap.frag.

shader_type spatial;
render_mode depth_draw_opaque, cull_back, diffuse_burley, specular_disabled;

#include "res://addons/libmaszyna/legacy/materials/types/detail_normalmap.gdshaderinc"
