#pragma once
#include <glm/glm.hpp>
#include "AABox.h"
struct Ray
{
	glm::vec3 Pos;
	glm::vec3 Dir;
	bool Intersect(const AABox& box, float& tmin, float& tmax) const;
	glm::vec3 Traverse(float t) const;
};

