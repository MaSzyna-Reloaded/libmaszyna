// What the point and the glare of a free spotlight share (opengl33renderer.cpp:4377-4396)

// A submodel is drawn only closer than its range (opengl33renderer.cpp:4292); none, none at all
bool free_spotlight_in_range(float square_distance, float range) {
	return square_distance < range * range;
}

// The light's centre moves from 5 to 25 cm along the light as the camera goes out to the range
const float LIGHT_CENTER_OFFSET_NEAR = 0.05;
const float LIGHT_CENTER_OFFSET_FAR = 0.25;

float free_spotlight_square_distance(mat4 modelview) {
	vec3 origin_view = (modelview * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
	return dot(origin_view, origin_view);
}

vec4 free_spotlight_center(mat4 modelview, float square_distance, float range) {
	float offset = mix(LIGHT_CENTER_OFFSET_NEAR, LIGHT_CENTER_OFFSET_FAR, square_distance / (range * range));
	return modelview * vec4(0.0, 0.0, -offset, 1.0);
}

// fCosViewAngle: the cosine between the light's axis and the direction to the camera
float free_spotlight_cos_view(mat4 modelview, vec4 center_view) {
	vec3 axis_point = (modelview * vec4(0.0, 0.0, -1.0, 1.0)).xyz;
	return dot(normalize(axis_point - center_view.xyz), normalize(-center_view.xyz));
}

// fCosViewAngle against the cone: 1 inside the hotspot, 0 outside the falloff
float free_spotlight_angle_factor(mat4 modelview, vec4 center_view, float cos_hotspot, float cos_falloff) {
	float cos_view = free_spotlight_cos_view(modelview, center_view);
	if (cos_view <= cos_falloff) {
		return 0.0;
	}
	return clamp((cos_view - cos_falloff) / (cos_hotspot - cos_falloff), 0.0, 1.0);
}
