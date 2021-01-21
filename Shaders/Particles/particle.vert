#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 proj;
    mat4 view;
} ubo;


layout (location = 0) in vec3 Pos;
layout (location = 1) in float Size;
layout (location = 2) in vec3 Velocity;
layout (location = 3) in float Alpha;

layout (location = 0) out vec4 outColor;

out gl_PerVertex
{
	vec4 gl_Position;
	float gl_PointSize;
};

void main () 
{
  vec4 position = ubo.proj * ubo.view * vec4(Pos, 1.0);
  gl_Position = position;
  gl_PointSize = Size;
  outColor = vec4(Velocity, Alpha);
}
