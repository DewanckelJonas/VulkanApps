#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 normal;

layout(location = 0) out vec4 outColor;

void main() {

	float diff = max(dot(normal, normalize(vec3(1,-1,0))), 0.0);
    outColor = vec4(fragColor, 1.0);
}