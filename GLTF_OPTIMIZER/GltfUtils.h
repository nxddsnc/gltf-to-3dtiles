#pragma once
#include "tiny_gltf.h"
#include "vcg\math\matrix44.h"
#pragma once
using namespace vcg;

// TODO: Add a cpp file
static void GetNodeMeshIdx(tinygltf::Model* model, tinygltf::Node* node, std::vector<int>& meshIdxs)
{
	if (node->children.size() > 0)
	{
		for (int i = 0; i < node->children.size(); ++i)
		{
			GetNodeMeshIdx(model, &(model->nodes[node->children[i]]), meshIdxs);
		}
	}
	if (node->mesh != -1)
	{
		meshIdxs.push_back(node->mesh);
	}
}

struct MeshInfo
{
    MeshInfo()
    {
        matrix = NULL;
    }
    MeshInfo(const MeshInfo & meshInfo)
    {
        if (meshInfo.matrix != NULL)
        {
            matrix = new Matrix44f(*(meshInfo.matrix));
        }
        else
        {
            matrix = NULL;
        }
        meshIdx = meshInfo.meshIdx;
    }
    ~MeshInfo()
    {
        if (matrix != NULL)
        {
            delete matrix;
            matrix = NULL;
        }
    }
    int meshIdx;
    vcg::Matrix44f* matrix;
};

static void GetNodeMeshInfos(tinygltf::Model* model, tinygltf::Node* node, std::vector<MeshInfo>& meshInfos, Matrix44f* parentMatrix=NULL)
{
    if (node->matrix.size() > 0 || parentMatrix != NULL)
    {
        Matrix44f matrix;
        matrix.SetIdentity();
        if (node->matrix.size() == 16)
        {
            //matrix = Matrix44f(node->matrix.data());
            for (int i = 0; i < 16; ++i)
            {
                matrix.V()[i] = node->matrix[i];
            }
            matrix = Transpose(matrix);
        }
        if (parentMatrix != NULL)
        {
            matrix = matrix * (*parentMatrix);
        }

        if (node->children.size() > 0)
        {
            for (int i = 0; i < node->children.size(); ++i)
            {
                GetNodeMeshInfos(model, &(model->nodes[node->children[i]]), meshInfos, &matrix);
            }
        }
        if (node->mesh != -1)
        {
            MeshInfo meshInfo;
            meshInfo.matrix = new Matrix44f(matrix);
            meshInfo.meshIdx = node->mesh;
            meshInfos.push_back(meshInfo);
        }
    }
    else
    {
        if (node->children.size() > 0)
        {
            for (int i = 0; i < node->children.size(); ++i)
            {
                GetNodeMeshInfos(model, &(model->nodes[node->children[i]]), meshInfos, NULL);
            }
        }
        if (node->mesh != -1)
        {
            MeshInfo meshInfo;
            meshInfo.meshIdx = node->mesh;
            meshInfos.push_back(meshInfo);
        }
    }
}

