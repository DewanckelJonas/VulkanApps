#include "VertexTypes.h"

size_t GetVertexTypeSize(VertexAttribute type)
{
	switch (type)
	{
	case VertexAttribute::PADDINGFLOAT:
	case VertexAttribute::FLOAT:
		return sizeof(float);
	case VertexAttribute::UV:
	case VertexAttribute::PADDINGVEC2:
	case VertexAttribute::VEC2:
		return 2 * sizeof(float);
	case VertexAttribute::COLOR:
	case VertexAttribute::PADDINGVEC4:
	case VertexAttribute::VEC4:
		return 4 * sizeof(float);
	default:
		return 3 * sizeof(float);
	}
}

size_t GetStride(const std::vector<VertexAttribute>& attributes)
{
	size_t stride{};
	for (VertexAttribute attribute : attributes)
	{
		stride += GetVertexTypeSize(attribute);
	}
	return stride;
}

bool IsAffectedByTransform(VertexAttribute type)
{
	switch (type)
	{
	//case VertexAttribute::NORMAL:
	case VertexAttribute::POSITION:
	case VertexAttribute::TANGENT:
	case VertexAttribute::BITANGENT:
		return true;
	default:
		return false;
		break;
	}
}
