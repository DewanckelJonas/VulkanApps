#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inColor;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec4 outColor;

void main() {
	vec3 lightDir = {0.5f ,0.3f, 0.8f};
	lightDir = normalize(lightDir);
	float ambient = 0.1f;
    float diffuse = max(dot(inNormal, lightDir), 0.0f);
    const vec3 constColor = {1.f, 1.f, 1.f};
	outColor = vec4(min(1.f, diffuse+ambient)*constColor, 1.f);
}