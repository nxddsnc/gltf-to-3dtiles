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

void MergeMesh::DoMerge()
{
	Node* root = &(m_pModel->nodes[m_pModel->nodes.size() - 1]);
	
	for (int i = 0; i < root->children.size(); ++i)
	{
		Node* node = &(m_pModel->nodes[root->children[i]]);
		std::vector<int> meshIdxs;
        GetNodeMeshIdx(m_pModel, node, meshIdxs);
        
        for (int j = 0; j < meshIdxs.size(); ++j)
        {
            Mesh* mesh = &(m_pModel->meshes[meshIdxs[j]]);
            int materialIdx = mesh->primitives[0].material;
            Material material = m_pModel->materials[materialIdx];
            if (m_materialMeshMap.count(material) > 0)
            {
                m_materialMeshMap.at(material).push_back(mesh);
            }
            else
            {
                std::vector<Mesh*> meshes;
                meshes.push_back(mesh);
                m_materialMeshMap.insert(make_pair(material, meshes));
            }
        }
	}
}