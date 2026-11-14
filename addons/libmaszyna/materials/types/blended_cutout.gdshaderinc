// What a blended surface leaves to its opaque one: the original's translucent pass draws only the
// texels below the material's "opacity:" - it passes it negated and the shader drops the texels at
// and above it (opengl33renderer.cpp:2247-2251, mat_default.frag:32) - so a vehicle's glass,
// drawn of its skin in the body's own plane, draws the glass and not the paint the opaque pass has
// drawn there already (MaszynaMaterialFactory: the material's opacity, 0 - none dropped - without one)
uniform float blended_opacity = 0.0;

// whether a texel of alpha belongs to the opaque pass, not this one
bool maszyna_blended_cut(float alpha) {
	return blended_opacity > 0.0 && alpha >= blended_opacity;
}
