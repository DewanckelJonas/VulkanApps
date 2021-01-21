#include "Ray.h"

bool Ray::Intersect(const AABox& box, float& tenter, float& texit) const
{
	float txmin = 0;
	float tymin = 0;
	float tzmin = 0;
	float txmax = 0;
	float tymax = 0;
	float tzmax = 0;
	if(abs(Dir.x) > FLT_EPSILON)
	{
		txmin = (box.Position.x - Pos.x) / Dir.x;
		txmax = (box.Position.x + box.Extent.x - Pos.x) / Dir.x;
	}
	if (abs(Dir.y) > FLT_EPSILON)
	{
		tymin = (box.Position.y - Pos.y) / Dir.y;
		tymax = (box.Position.y + box.Extent.y - Pos.y) / Dir.y;
	}
	if (abs(Dir.z) > FLT_EPSILON)
	{
		tzmin = (box.Position.z - Pos.z) / Dir.z;
		tzmax = (box.Position.z + box.Extent.z - Pos.z) / Dir.z;
	}

	float txenter = glm::min(txmin, txmax);
	float txexit = glm::max(txmin, txmax);
	float tyenter = glm::min(tymin, tymax);
	float tyexit = glm::max(tymin, tymax);
	float tzenter = glm::min(tzmin, tzmax);
	float tzexit = glm::max(tzmin, tzmax);
	tenter = glm::max(txenter, glm::max(tyenter, tzenter));
	texit = glm::min(txexit, glm::min(tyexit, tzexit));
	return texit > tenter;
}

glm::vec3 Ray::Traverse(float t) const
{
	return Pos+(t*Dir);
}
