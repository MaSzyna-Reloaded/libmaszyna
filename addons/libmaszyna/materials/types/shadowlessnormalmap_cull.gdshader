// The shadowlessnormalmap material type two-sided (MaterialOptions.cull_disabled): a variant in a
// file of its own (MaszynaMaterialFactory.SHADER_VARIANTS), so that the export's shader baker bakes
// it
shader_type spatial;
render_mode depth_draw_opaque, cull_disabled, diffuse_burley, specular_disabled;

#include "res://addons/libmaszyna/materials/types/shadowlessnormalmap.gdshaderinc"
