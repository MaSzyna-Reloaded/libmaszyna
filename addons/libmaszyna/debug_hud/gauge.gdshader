shader_type canvas_item;


uniform vec2 center = vec2(0.0, 0.0);
uniform float radius = 1.0;
uniform float feather = 0.005;

void vertex() {
	// Called for every vertex the material is visible on.
}

float circle(vec2 position)
{
	return smoothstep(radius, radius + feather, dot(position, position) * 4.0);
}


void fragment() {
	COLOR.rgba = vec4( circle(UV-center) );
}

//void light() {
	// Called for every pixel for every light affecting the CanvasItem.
	// Uncomment to replace the default light processing function with this one.
//}
