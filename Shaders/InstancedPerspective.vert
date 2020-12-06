#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 proj;
    mat4 view;
} ubo;

layout(binding = 1) buffer StorageBufferObject{bool terrain[];} sbo; 

layout(location = 0) in vec3 inPosition;
//layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main() {
    vec4 position = ubo.proj * ubo.view * vec4(inPosition.x + 6*(gl_InstanceIndex%100), inPosition.y + 6 *(gl_InstanceIndex/10000), inPosition.z + 6*((gl_InstanceIndex%10000)/100), 1.0);
	gl_Position = position;
    fragColor = vec3(6*(gl_InstanceIndex%100)/100.f, 6*(gl_InstanceIndex/10000)/100.f, 6*((gl_InstanceIndex%10000)/100)/100.f);
}