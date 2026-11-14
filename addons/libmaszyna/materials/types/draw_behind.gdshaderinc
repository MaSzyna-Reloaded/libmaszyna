// A model drawn a little farther from the camera than it lies (reloaded_depth_behind [m], laid on its
// instances by the vehicle rendering): a vehicle's low-poly interior, part of which lies in the
// body's own plane - the original draws it first and the body over it (GL_LEQUAL,
// opengl33renderer.cpp:3179-3192), so where they meet the body is seen, not the two in turn

instance uniform float reloaded_depth_behind = 0.0;

// vertex moved away from camera_position by depth [m], in the model's own space
vec3 maszyna_draw_behind(vec3 vertex, mat4 model_matrix, vec3 camera_position, float depth) {
	if (depth <= 0.0) {
		return vertex;
	}
	vec3 away = normalize((model_matrix * vec4(vertex, 1.0)).xyz - camera_position);
	return vertex + inverse(mat3(model_matrix)) * away * depth;
}
