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
	PADDINGFLOAT,
	PADDINGVEC4
};

size_t GetVertexTypeSize(VertexAttribute type);
size_t GetStride(const std::vector<VertexAttribute>& attributes);
bool IsAffectedByTransform(VertexAttribute type);
