#pragma once
#include <vector>
#include "MyMesh.h"
#include <unordered_map>

namespace tinygltf
{
    class Model;
    class Node;
    class TinyGLTF;
	class Mesh;
	class Material;
}
class LodExporter
{
public:
    LodExporter(tinygltf::Model* model, std::vector<MyMesh*> myMeshes, tinygltf::TinyGLTF* tinyGLTF);
    ~LodExporter();
    void ExportLods(std::vector<LodInfo> lodInfos, int level);
private:
    void getMeshIdxs(std::vector<int> nodeIdxs, std::vector<int>& meshIdxs);
    void traverseNode(tinygltf::Node* node, std::vector<int>& meshIdxs);

	int addNode(tinygltf::Node* node);
	int addMesh(tinygltf::Mesh* mesh, MyMesh* myMesh);
	int addMaterial(int material);
	void addPrimitive(Primitive* primitive, Mesh* mesh, MyMesh* myMesh); 
	int addAccessor();

	std::unordered_map<int, int> m_materialCache; // map between old material and new material;
private:
    tinygltf::Model* m_pModel;
	tinygltf::Model* m_pNewModel;
    std::vector<MyMesh*> m_myMeshes;
    TriEdgeCollapseQuadricParameter* m_qparams;
    tinygltf::TinyGLTF* m_tinyGTLF;
};

