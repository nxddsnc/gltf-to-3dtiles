#pragma once
#include <vector>
#include <unordered_map>
#include <map>
#include "MyMesh.h"
#define MATERIAL_EPS 0.01
#define MIN_FACE_NUM 10 // If face number is less then this, don't decimate then. 

//
//namespace tinygltf
//{
//	class Model;
//}

// If the hash compare goes slow, optimize this function.

class MeshOptimizer
{
public:
	MeshOptimizer(std::vector<MyMeshInfo> meshInfos);
	~MeshOptimizer();

	void DoMerge();
    float DoDecimation(float targetPercetage);
	std::vector<MyMeshInfo> GetMergedMeshInfos() { return m_mergeMeshInfos; }
private:
    void mergeSameMaterialMeshes(tinygltf::Material* material, std::vector<MyMesh*> meshes);
private:

    std::vector<MyMeshInfo> m_meshInfos;
	std::vector<MyMeshInfo> m_mergeMeshInfos;

    TriEdgeCollapseQuadricParameter* m_pParams;
};