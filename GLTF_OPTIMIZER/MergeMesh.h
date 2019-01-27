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
	//void Initialize(Model* pModel);
private :
	void traverseNode(tinygltf::Node* node, std::vector<int>& meshIdxs);
private:
	tinygltf::Model* m_pModel;
	std::vector<MyMesh*> m_myMeshes;
};

