#include "MathExtension.h"
void TransformVector(std::vector<glm::vec3>& vertices, const glm::mat4x4& transform)
{
	for (glm::vec3& vertex : vertices)
	{
		vertex = transform * glm::vec4(vertex, 1.f);
	}
}

void TransformVector(std::vector<glm::vec4>& vertices, const glm::mat4x4& transform)
{
	for (glm::vec4& vertex : vertices)
	{
		vertex = transform * vertex;
	}
}