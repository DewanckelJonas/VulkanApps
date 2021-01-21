#pragma once
#include <glm/glm.hpp>
struct AABox
{
	glm::vec3 Position;
	glm::vec3 Extent;
	glm::vec3 GetCenter() { return Position + (Extent / 2.f); }
	glm::vec3 SetCenter(glm::vec3 center) { Position = center - (Extent / 2.f); }

};

