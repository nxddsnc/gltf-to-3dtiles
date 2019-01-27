#pragma once
#include <vector>
#include "tiny_gltf.h"
//
//namespace tinygltf
//{
//	class Model;
//}

class MyMesh;

class MergeMesh
{
public:
	MergeMesh(tinygltf::Model* model, std::vector<MyMesh*> myMeshes);
	~MergeMesh();

	void DoMerge();
private :
	void getMeshIdxs(std::vector<int> nodeIdxs, std::vector<int>& meshIdxs);
	void traverseNode(tinygltf::Node* node, std::vector<int>& meshIdxs);
private:
	tinygltf::Model* m_pModel;
	std::vector<MyMesh*> m_myMeshes;
};

