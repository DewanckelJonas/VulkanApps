#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 proj;
    mat4 view;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 outNormal;

void main() {
    vec4 position = ubo.proj * ubo.view * vec4(inPosition, 1.0);
	gl_Position = position;
    fragColor = inPosition/100.f;
}