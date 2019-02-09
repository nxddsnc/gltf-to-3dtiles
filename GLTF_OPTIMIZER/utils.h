#pragma once
#include "vcg\math\matrix44.h"
#include "MyMesh.h"
#include <unordered_map>
using namespace vcg;
namespace tinygltf
{
	struct Model;
	struct Node;
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

void ConcatMyMesh(MyMesh* dest, MyMesh* src);

void GetNodeMeshInfos(tinygltf::Model* model, tinygltf::Node* node, std::vector<MeshInfo>& meshInfos, Matrix44f* parentMatrix = NULL);

