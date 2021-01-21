#pragma once
#include <vector>
enum class VertexAttribute
{
	POSITION,
	NORMAL,
	COLOR,
	UV,
	TANGENT,
	BITANGENT,
	FLOAT,
	VEC2,
	VEC3,
	VEC4,
	PADDINGFLOAT,
	PADDINGVEC2,
	PADDINGVEC3,
	PADDINGVEC4
};

size_t GetVertexTypeSize(VertexAttribute type);
size_t GetStride(const std::vector<VertexAttribute>& attributes);
bool IsAffectedByTransform(VertexAttribute type);
