#include "MergeMesh.h"
#include "MyMesh.h"
#include <algorithm>
#include "vcg/complex/algorithms/clean.h"
#include "utils.h"
#include "tiny_gltf.h"
using namespace tinygltf;
using namespace std;
struct material_hash_fn {
	unsigned long operator()(const tinygltf::Material& material) const {
		unsigned long hash = 0;
		hash += 31 * material.values.size();
		std::map<std::string, tinygltf::Parameter>::const_iterator it = material.values.begin();
		// assume the color values are from 0-1.
		for (it = material.values.begin(); it != material.values.end(); ++it)
		{
			if (it->second.has_number_value)
			{
				hash += 31 * it->second.number_value;
			}
			else
			{
				for (int i = 0; i < it->second.number_array.size(); ++i)
				{
					hash += int(it->second.number_array[i] * 255.0) * 31;
				}
			}
		}
		return hash;
	}
};

struct material_equal_fn {
	bool operator()(const tinygltf::Material& m1, const tinygltf::Material& m2) const {
		if (m1.values.size() != m2.values.size())
		{
			return false;
		}
		std::map<std::string, tinygltf::Parameter>::const_iterator it;
		for (it = m1.values.begin(); it != m1.values.end(); ++it)
		{
			std::string name = it->first;
			if (m2.values.count(name) < 1)
				return false;
			tinygltf::Parameter values = m2.values.at(name);

			if (it->second.has_number_value != values.has_number_value ||
				it->second.bool_value != values.bool_value)
			{
				return false;
			}
			if (std::abs(it->second.number_value - values.number_value) > MATERIAL_EPS)
			{
				return false;
			}
			if (it->second.number_array.size() != values.number_array.size())
			{
				return false;
			}
			for (int i = 0; i < it->second.number_array.size(); ++i)
			{
				if (std::abs(it->second.number_array[i] - values.number_array[i]) > MATERIAL_EPS)
				{
					return false;
				}
			}
		}
		return true;
	}
};

MeshOptimizer::MeshOptimizer(std::vector<MyMeshInfo> meshInfos)
{
    m_meshInfos = meshInfos;

    m_pParams = new TriEdgeCollapseQuadricParameter();
    m_pParams->QualityThr = .3;
    m_pParams->PreserveBoundary = false; // Perserve mesh boundary
    m_pParams->PreserveTopology = false;
}

MeshOptimizer::~MeshOptimizer()
{
	//for (int i = 0; i < m_mergeMeshInfos.size(); ++i)
	//{
	//	if (m_mergeMeshInfos[i].myMesh != NULL)
	//	{
	//		delete m_mergeMeshInfos[i].myMesh;
	//		m_mergeMeshInfos[i].myMesh = NULL;
	//	}
	//}
	if (m_pParams != NULL)
	{
		delete m_pParams;
		m_pParams = NULL;
	}
}

struct mesh_compare_fn
{
    inline bool operator() (const MyMesh* myMesh1, const MyMesh* myMesh2)
    {
        return myMesh1->vn < myMesh2->vn;
    }
};

float MeshOptimizer::DoDecimation(float targetPercentage)
{
	float geometryError = 0;
	for (int i = 0; i < m_mergeMeshInfos.size(); ++i)
	{
		MyMesh* myMesh = m_mergeMeshInfos[i].myMesh;
		// decimator initialization
		vcg::LocalOptimization<MyMesh> deciSession(*myMesh, m_pParams);
		deciSession.Init<MyTriEdgeCollapse>();
		// FIXME: If the mesh bbox is large and it's face number is ralatively few, we should not do decimation.
		uint32_t finalSize = myMesh->fn * targetPercentage;
		finalSize = finalSize < MIN_FACE_NUM ? MIN_FACE_NUM : finalSize;
		deciSession.SetTargetSimplices(finalSize); // Target face number;
		deciSession.SetTimeBudget(0.5f); // Time budget for each cycle
		deciSession.SetTargetOperations(100000);
		int maxTry = 100;
		int currentTry = 0;
		do
		{
			deciSession.DoOptimization();
			currentTry++;
		} while (myMesh->fn > finalSize && currentTry < maxTry);

		geometryError += deciSession.currMetric;

		tri::Clean<MyMesh>::RemoveUnreferencedVertex(*myMesh);
	}
	return geometryError;
}

void MeshOptimizer::DoMerge()
{
	std::unordered_map<tinygltf::Material, std::vector<MyMesh*>, material_hash_fn, material_equal_fn> m_materialMeshMap;
	for (int i = 0; i < m_meshInfos.size(); ++i)
    {
        if (m_materialMeshMap.count(*m_meshInfos[i].material) > 0)
        {
            m_materialMeshMap.at(*m_meshInfos[i].material).push_back(m_meshInfos[i].myMesh);
        }
        else
        {
            std::vector<MyMesh*> myMeshesToMerge;
            myMeshesToMerge.push_back(m_meshInfos[i].myMesh);
            m_materialMeshMap.insert(make_pair(*m_meshInfos[i].material, myMeshesToMerge));
        }
    }

    std::unordered_map<tinygltf::Material, std::vector<MyMesh*>, material_hash_fn, material_equal_fn>::iterator it;
    for (it = m_materialMeshMap.begin(); it != m_materialMeshMap.end(); ++it)
    {
        std::vector<MyMesh*> myMeshes = it->second;

        std::sort(myMeshes.begin(), myMeshes.end(), mesh_compare_fn());

        mergeSameMaterialMeshes((Material*)&(it->first), myMeshes);
    }
}


void MeshOptimizer::mergeSameMaterialMeshes(Material* material, std::vector<MyMesh*> myMeshes)
{
	int totalVertex = 0;
	int totalFace = 0;
	std::vector<MyMesh*> meshesToMerge;
	MyMesh* mergedMesh = new MyMesh();
	for (int i = 0; i < myMeshes.size(); ++i)
	{
		MyMesh* myMesh = myMeshes[i];
		// FIXME: Maybe not neccessary to limit the vertex number since we will do decimation later anyway.
		if (totalVertex + myMesh->vn > 65536 || (totalVertex + myMesh->vn < 65536 && i == myMeshes.size() - 1))
		{
			if (totalVertex + myMesh->vn < 65536 && i == myMeshes.size() - 1)
			{
				ConcatMyMesh(mergedMesh, myMesh);
				totalVertex += myMesh->vn;
				totalFace += myMesh->fn;
			}
			MyMeshInfo meshInfo;
			meshInfo.material = material;
			meshInfo.myMesh = mergedMesh;
			m_mergeMeshInfos.push_back(meshInfo);
			totalVertex = 0;
			totalFace = 0;
		}

		ConcatMyMesh(mergedMesh, myMesh);

		totalVertex += myMesh->vn;
		totalFace += myMesh->fn;
	}
}