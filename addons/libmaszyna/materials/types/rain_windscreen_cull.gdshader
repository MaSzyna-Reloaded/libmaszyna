// The rain_windscreen material type two-sided (MaterialOptions.cull_disabled): a variant in a file
// of its own (MaszynaMaterialFactory.SHADER_VARIANTS), so that the export's shader baker bakes it
shader_type spatial;
render_mode blend_mix, cull_disabled, diffuse_burley, specular_schlick_ggx;

#include "res://addons/libmaszyna/materials/types/rain_windscreen.gdshaderinc"
