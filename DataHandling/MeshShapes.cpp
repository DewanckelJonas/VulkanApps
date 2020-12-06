#include "MeshShapes.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <Base/MathExtension.h>

void GeneratePlaneMeshVertexData(std::vector<glm::vec3>& positions, std::vector<glm::vec2>& uvs, float width, float height, const glm::ivec2& subdivision)
{
	size_t horVertices = size_t(subdivision.x) + 2;
	size_t vertVertices = size_t(subdivision.y) + 2;

	positions.clear();
	uvs.clear();
	positions.reserve(horVertices * vertVertices);
	uvs.reserve(horVertices * vertVertices);

	glm::vec2 topLeft{ -width / 2.f, height / 2.f };
	glm::vec2 subdivisionOffset{ width / (horVertices - 1) , -height / (vertVertices - 1) };

	//Vertical subdivision
	for (size_t y = 0; y < vertVertices; y++)
	{
		//Horizontal subdivision
		for (size_t x = 0; x < horVertices; x++)
		{
			positions.push_back({ topLeft + (glm::vec2(float(x), float(y)) * subdivisionOffset), 0.f });
			uvs.push_back({ float(x) / (horVertices - 1), float(y) / (vertVertices - 1) });
		}
	}
}

void OffsetIndices(std::vector<uint32_t>& indices, size_t offset)
{
	for(uint32_t& index : indices)
	{
		index += offset;
	}
}

void GeneratePlaneMeshIndexData(std::vector<uint32_t>& indices, const glm::ivec2& subdivision, size_t indexOffset = 0)
{
	size_t horVertices = size_t(subdivision.x) + 2;
	size_t vertVertices = size_t(subdivision.y) + 2;
	//Generate indices
	for (size_t y = 0; y < vertVertices - 1; y++)
	{
		for (size_t x = 0; x < horVertices - 1; x++)
		{
			size_t startIndex{ uint32_t(x + (y * horVertices)) + indexOffset };
			//TopLeft triangle
			indices.push_back(startIndex);
			indices.push_back(startIndex + 1);
			indices.push_back(horVertices + startIndex);

			//BottomRight triangle
			indices.push_back(horVertices + startIndex);
			indices.push_back(startIndex + 1);
			indices.push_back(horVertices + startIndex + 1);
		}
	}
}

Mesh* CreatePlaneMesh(float width, float height, const glm::ivec2& subdivision, const glm::vec4& color)
{
	Mesh* pPlane = new Mesh();
	std::vector<glm::vec3> positions{};
	std::vector<glm::vec2> uvs{};
	std::vector<uint32_t> indices{};

	GeneratePlaneMeshVertexData(positions, uvs, width, height, subdivision);
	GeneratePlaneMeshIndexData(indices, subdivision);

	pPlane->AddVertexAttribute(VertexAttribute::POSITION, positions);
	pPlane->AddVertexAttribute(VertexAttribute::UV, uvs);
	pPlane->AddVertexAttribute(VertexAttribute::NORMAL, std::vector<glm::vec3>{ {0, 0, -1} });
	pPlane->AddVertexAttribute(VertexAttribute::COLOR, std::vector<glm::vec4>{color});
	pPlane->SetFillVertexAttribute(VertexAttribute::NORMAL, true);
	pPlane->SetFillVertexAttribute(VertexAttribute::COLOR, true);
	pPlane->SetIndices(indices);
	return pPlane;
}

Mesh* CreateCubeMesh(float size, glm::ivec2 subdivision, glm::vec4 color)
{
	return CreateRectBox(size, size, size, subdivision, color);
}

Mesh* CreateRectBox(float width, float height, float depth, glm::ivec2 subdivision, glm::vec4 color)
{
	std::vector<glm::vec3> frontPlanePositions;
	std::vector<glm::vec2> frontPlaneUVs;
	std::vector<uint32_t> frontPlaneIndices;
	std::vector<glm::vec3> sidePlanePositions;
	std::vector<glm::vec2> sidePlaneUVs;
	std::vector<uint32_t> sidePlaneIndices;
	std::vector<glm::vec3> topPlanePositions;
	std::vector<glm::vec2> topPlaneUVs;
	std::vector<uint32_t> topPlaneIndices;

	GeneratePlaneMeshVertexData(frontPlanePositions, frontPlaneUVs, width, height, subdivision);
	GeneratePlaneMeshVertexData(sidePlanePositions, sidePlaneUVs, depth, height, subdivision);
	GeneratePlaneMeshVertexData(topPlanePositions, topPlaneUVs, width, depth, subdivision);

	GeneratePlaneMeshIndexData(frontPlaneIndices, subdivision);
	GeneratePlaneMeshIndexData(sidePlaneIndices, subdivision);
	GeneratePlaneMeshIndexData(topPlaneIndices, subdivision);

	std::vector<glm::vec3> positions;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;
	std::vector<uint32_t> indices;

	positions.reserve(frontPlanePositions.size() * 2 + sidePlanePositions.size() * 2 + topPlanePositions.size() * 2);
	uvs.reserve(frontPlaneUVs.size() * 2 + sidePlaneUVs.size() * 2 + topPlaneUVs.size() * 2);
	indices.reserve(frontPlaneIndices.size() * 2 + sidePlaneIndices.size() * 2 + topPlaneIndices.size() * 2); 
	normals.resize(frontPlanePositions.size() * 2 + sidePlanePositions.size() * 2 + topPlanePositions.size() * 2);
	auto normalIt = normals.begin();

	//front plane
	glm::mat4x4 transMatrix = glm::translate(glm::mat4x4(1.f), { 0, 0, -depth/2.f });
	TransformVector(frontPlanePositions, transMatrix);
	positions.insert(positions.end(), frontPlanePositions.begin(), frontPlanePositions.end());
	uvs.insert(uvs.end(), frontPlaneUVs.begin(), frontPlaneUVs.end());
	indices.insert(indices.end(), frontPlaneIndices.begin(), frontPlaneIndices.end());
	std::fill(normalIt, normalIt + frontPlanePositions.size(), glm::vec3{ 0, 0, -1 });
	normalIt += frontPlanePositions.size();

	//back plane
	glm::mat4x4 rotMatrix = glm::rotate(glm::mat4x4(1.f), glm::pi<float>(), { 0, 1, 0 });
	TransformVector(frontPlanePositions, rotMatrix);
	positions.insert(positions.end(), frontPlanePositions.begin(), frontPlanePositions.end());
	uvs.insert(uvs.end(), frontPlaneUVs.begin(), frontPlaneUVs.end());
	OffsetIndices(frontPlaneIndices, frontPlanePositions.size());
	indices.insert(indices.end(), frontPlaneIndices.begin(), frontPlaneIndices.end());
	std::fill(normalIt, normalIt + frontPlanePositions.size(), glm::vec3{ 0 , 0 , 1 });
	normalIt += frontPlanePositions.size();

	//right plane
	transMatrix = glm::translate(glm::mat4x4(1.f), { width / 2.f, 0, 0 });
	rotMatrix = glm::rotate(glm::mat4x4(1.f), -glm::pi<float>() / 2.f, { 0, 1, 0 });
	TransformVector(sidePlanePositions, transMatrix * rotMatrix);
	positions.insert(positions.end(), sidePlanePositions.begin(), sidePlanePositions.end());
	uvs.insert(uvs.end(), sidePlaneUVs.begin(), sidePlaneUVs.end());
	OffsetIndices(sidePlaneIndices, frontPlanePositions.size()*2);
	indices.insert(indices.end(), sidePlaneIndices.begin(), sidePlaneIndices.end());
	std::fill(normalIt, normalIt + sidePlanePositions.size(), glm::vec3{ 1, 0, 0 });
	normalIt += sidePlanePositions.size();

	//left plane
	rotMatrix = glm::rotate(glm::mat4x4(1.f), glm::pi<float>(), { 0, 1, 0 });
	TransformVector(sidePlanePositions, rotMatrix);
	positions.insert(positions.end(), sidePlanePositions.begin(), sidePlanePositions.end());
	uvs.insert(uvs.end(), sidePlaneUVs.begin(), sidePlaneUVs.end());
	OffsetIndices(sidePlaneIndices, sidePlanePositions.size());
	indices.insert(indices.end(), sidePlaneIndices.begin(), sidePlaneIndices.end());
	std::fill(normalIt, normalIt + sidePlanePositions.size(), glm::vec3{ -1, 0, 0 });
	normalIt += sidePlanePositions.size();

	//top plane
	transMatrix = glm::translate(glm::mat4x4(1.f), { 0, height / 2.f, 0 });
	rotMatrix = glm::rotate(glm::mat4x4(1.f), glm::pi<float>() / 2.f, { 1, 0, 0 });
	TransformVector(topPlanePositions, transMatrix * rotMatrix);
	positions.insert(positions.end(), topPlanePositions.begin(), topPlanePositions.end());
	uvs.insert(uvs.end(), topPlaneUVs.begin(), topPlaneUVs.end());
	OffsetIndices(topPlaneIndices, frontPlanePositions.size()*2 + sidePlanePositions.size()*2);
	indices.insert(indices.end(), topPlaneIndices.begin(), topPlaneIndices.end());
	std::fill(normalIt, normalIt + topPlanePositions.size(), glm::vec3{ 0, 1, 0 });
	normalIt += topPlanePositions.size();

	//bottom plane
	rotMatrix = glm::rotate(glm::mat4x4(1.f), glm::pi<float>(), { 1,0,0 });
	TransformVector(topPlanePositions, rotMatrix);
	positions.insert(positions.end(), topPlanePositions.begin(), topPlanePositions.end());
	uvs.insert(uvs.end(), topPlaneUVs.begin(), topPlaneUVs.end());
	OffsetIndices(topPlaneIndices, topPlanePositions.size());
	indices.insert(indices.end(), topPlaneIndices.begin(), topPlaneIndices.end());
	std::fill(normalIt, normalIt + topPlanePositions.size(), glm::vec3{ 0, -1, 0 });

	Mesh* pCubeMesh = new Mesh{};
	pCubeMesh->AddVertexAttribute(VertexAttribute::POSITION, positions);
	pCubeMesh->AddVertexAttribute(VertexAttribute::NORMAL, normals);
	pCubeMesh->AddVertexAttribute(VertexAttribute::UV, uvs);
	pCubeMesh->AddVertexAttribute(VertexAttribute::COLOR, std::vector<glm::vec4>{ color });
	pCubeMesh->SetFillVertexAttribute(VertexAttribute::COLOR, true);
	pCubeMesh->SetIndices(indices);

	return pCubeMesh;
}

