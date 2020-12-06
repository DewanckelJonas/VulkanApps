#pragma once
#include <glm/glm.hpp>
#include <vector>
void TransformVector(std::vector<glm::vec3>& vertices, const glm::mat4x4& transform);
void TransformVector(std::vector<glm::vec4>& vertices, const glm::mat4x4& transform);