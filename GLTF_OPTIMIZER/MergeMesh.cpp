#include "MergeMesh.h"
#include "MyMesh.h"
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

void MergeMesh::DoMerge()
{
	Node* root = &(m_pModel->nodes[0]);

	for (int i = 0; i < root->children.size(); ++i)
	{
		Node* node = &(m_pModel->nodes[root->children[i]]);
		

	}
}

void MergeMesh::getMeshIdxs(std::vector<int> nodeIdxs, std::vector<int>& meshIdxs)
{
	for (int i = 0; i < nodeIdxs.size(); ++i)
	{
		traverseNode(&(m_pModel->nodes[nodeIdxs[i]]), meshIdxs);
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
