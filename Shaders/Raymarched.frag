#version 450
#extension GL_ARB_separate_shader_objects : enable

#define MAX_STEPS 100
#define MAX_DIST 10000
#define SURF_DIST 0.01

layout(binding = 0) uniform UniformBufferObject {
    vec4 pos;
	vec4 forward;
	vec3 right;
	float time;
	vec3 up;
	float aspectRatio;
} ubo;

layout(binding = 1) buffer StorageBufferObject{bool terrain[];} sbo; 

layout(location = 0) in vec2 UV;
layout(location = 0) out vec4 outColor;

float smin( float a, float b, float k )
{
    float h = clamp( 0.5+0.5*(b-a)/k, 0.0, 1.0 );
    return mix( b, a, h ) - k*h*(1.0-h);
}

float DistToScene(vec3 pos)
{
	float minDist = MAX_DIST;
	for(int i = 0; i < 10; i++)
	{
		vec3 cubePos = vec3(2*(i%100), 5*sin(ubo.time+i)+2.5 *(i/10000), 2.5*((i%10000)/100));
		vec3 q = abs(cubePos-pos) - vec3(1,1,1);
		float dist = length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);
		minDist = smin(dist, minDist, 0.8f);
	}
	return minDist;
}

float RayMarch(vec3 origin, vec3 dir)
{
	float dist = 0;
	for(int i=0; i < MAX_STEPS; i++)
	{
		vec3 pos = origin + dist * dir;
		float sceneDist = DistToScene(pos);
		dist += sceneDist;
		if(dist > MAX_DIST || sceneDist < SURF_DIST)
		break;
	}
	return dist;
}

void main() {
	vec3 eyePos = ubo.pos.xyz;
	vec3 lookDir = normalize(ubo.forward.xyz + ubo.aspectRatio*ubo.right.xyz*(UV.x-0.5f) + ubo.up.xyz*(UV.y-0.5f));
	float dist = RayMarch(eyePos, lookDir);
	vec3 color = vec3(1.f, 1.f, 1.f) - vec3(dist/MAX_DIST, dist/MAX_DIST, dist/MAX_DIST);
    outColor = vec4(color, 1.0);
}