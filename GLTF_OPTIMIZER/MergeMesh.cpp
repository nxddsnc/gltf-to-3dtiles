#include "MergeMesh.h"
#include "MyMesh.h"
#include <algorithm>
#include "vcg/complex/algorithms/clean.h"
#include "utils.h"
using namespace tinygltf;
using namespace std;

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
	for (int i = 0; i < m_mergeMeshInfos.size(); ++i)
	{
		if (m_mergeMeshInfos[i].myMesh != NULL)
		{
			delete m_mergeMeshInfos[i].myMesh;
			m_mergeMeshInfos[i].myMesh = NULL;
		}
	}
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