#include "MergeMesh.h"
#include "MyMesh.h"
#include "GltfUtils.h"
using namespace tinygltf;
using namespace std;

MergeMesh::MergeMesh(tinygltf::Model* model, std::vector<MyMesh*> myMeshes)
{
	m_pModel = model;
	m_myMeshes = myMeshes;
}


MergeMesh::~MergeMesh()
{
}
//
//void MergeMesh::Initialize(Model* pModel)
//{
//	Node* root = &(pModel->nodes[0]);
//	for (int i = 0; i < root->children.size(); ++i)
//	{
//
//	}
//}

void MergeMesh::DoMerge()
{
	Node* root = &(m_pModel->nodes[0]);
	
	for (int i = 0; i < root->children.size(); ++i)
	{
		Node* node = &(m_pModel->nodes[root->children[i]]);
		std::vector<int> meshIdxs;
		traverseNode(node, meshIdxs);
	}
}

void MergeMesh::traverseNode(tinygltf::Node* node, std::vector<int>& meshIdxs)
{
	if (node->children.size() > 0)
	{
		for (int i = 0; i < node->children.size(); ++i)
		{
			traverseNode(&(m_pModel->nodes[node->children[i]]), meshIdxs);
		}
	}
	if (node->mesh != -1)
	{
		meshIdxs.push_back(node->mesh);
	}
}
