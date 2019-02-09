#pragma once
#include "MyMesh.h"
#include <vector>
#include <unordered_map>

namespace tinygltf
{
	class Model;
	class Material;
	class Primitive;
}
enum AccessorType
{
	POSITION,
	NORMAL,
	UV,
	INDEX,
	BATCH_ID
};

class GltfExporter
{
public:
	GltfExporter(std::vector<MyMeshInfo> myMeshInfos, std::string bufferName);
	~GltfExporter();
	void ConstructNewModel();
	tinygltf::Model* GetNewModel() { return m_pNewModel; }
private:
	int addMesh(tinygltf::Material* material, MyMesh* myMesh);
	void addPrimitive(tinygltf::Primitive* primitive);
	int addAccessor(AccessorType type);
	int addBufferView(AccessorType type, size_t& byteOffset);
	int addBuffer(AccessorType type);
private:
	std::string m_bufferName;
	tinygltf::Model* m_pNewModel;
	std::vector<MyMeshInfo> m_myMeshInfos;
	float m_positionMax[3];
	float m_positionMin[3];
	std::unordered_map<MyVertex*, uint32_t> m_vertexUintMap;
	std::unordered_map<MyVertex*, uint16_t> m_vertexUshortMap;
	std::unordered_map<tinygltf::Material*, int> m_materialIndexMap;
	std::vector<unsigned char> m_currentAttributeBuffer;
	std::vector<unsigned char> m_currentIndexBuffer;
	std::vector<unsigned char> m_currentBatchIdBuffer;

	MyMesh* m_currentMesh;
};

