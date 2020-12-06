#pragma once
#include "Mesh.h"
Mesh* CreatePlaneMesh(float width = 1.f, float height = 1.f, const glm::ivec2& subdivision = { 0,0 }, const glm::vec4& color = {1,1,1,1});
Mesh* CreateCubeMesh(float size = 1.f, glm::ivec2 subdivision = { 0,0 }, glm::vec4 color = {1.f, 1.f, 1.f, 1.f});
Mesh* CreateRectBox(float width = 1.f, float height = 1.f, float depth = 1.f, glm::ivec2 subdivision = { 0,0 }, glm::vec4 color = {1.f, 1.f, 1.f, 1.f});
Mesh* CreateSphereMesh(float radius = 0.5f, glm::vec4 color = { 1.f, 1.f, 1.f, 1.f });
Mesh* CreateCapsuleMesh(float radius = 0.5f, float height = 1.f, glm::vec4 color = { 1.f, 1.f, 1.f, 1.f });
Mesh* CreateCylinderMesh(float radius = 0.5f, float height = 1.f, glm::vec4 color = { 1.f, 1.f, 1.f, 1.f });