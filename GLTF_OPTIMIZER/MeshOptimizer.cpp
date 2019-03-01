#include "MeshOptimizer.h"
#include "MyMesh.h"
#include <algorithm>
#include "vcg/complex/algorithms/clean.h"
#include "vcg/complex/algorithms/crease_cut.h"
#include <vcg/complex/algorithms/clustering.h>
#include<vcg/complex/algorithms/isotropic_remeshing.h>
#include<vcg/complex/algorithms/create/ball_pivoting.h>
#include "utils.h"
#include "tiny_gltf.h"
#include <unordered_map>
#include <ctime>
#include "globals.h"
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
    m_pParams->OptimalPlacement = false;
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

bool callback(int percent, const char *str)
{
    //cout << "str: " << str << " " << percent << "%\n";
    return true;
}
float MeshOptimizer::DoDecimation(float tileBoxMaxLength, bool remesh)
{
    //float geometryError = 0;
    //float maxBoxDiag = 0;
    int totalFaceCount = 0;
    for (int i = 0; i < m_mergeMeshInfos.size(); i++) 
    {
        totalFaceCount += m_mergeMeshInfos[i].myMesh->fn;
    }
    if (totalFaceCount == 0)
    {
        return 0.0f;
    }
    float decimationRatio = (float)g_settings.faceCountPerTile / (float)totalFaceCount;
    if (decimationRatio >= 1.0)
    {
        return 0;
    }
    int totalFaceBeforeSplit = 0;
    for (int i = 0; i < m_mergeMeshInfos.size(); ++i)
    {
        MyMesh* myMesh = m_mergeMeshInfos[i].myMesh;

        if (remesh)
        {
            MyMesh* sampleMesh = new MyMesh();
            VertexIterator vi = Allocator<MyMesh>::AddVertices(*sampleMesh, myMesh->vn);
            for (int ii = 0; ii < myMesh->vert.size(); ++ii)
            {
                if (myMesh->vert[ii].IsD())
                {
                    continue;
                }

                (*vi) = myMesh->vert[ii];
                ++vi;
            }

            float radius = tileBoxMaxLength / 10000;
            float minEdge = 0.001;
            float angle = M_PI / 4;
            tri::BallPivoting<MyMesh> ballPivoting = tri::BallPivoting<MyMesh>(*sampleMesh, radius, minEdge, angle);

            ballPivoting.BuildMesh(callback);

            delete myMesh;
            myMesh = sampleMesh;
            m_mergeMeshInfos[i].myMesh = myMesh;
        }
        //vcg::tri::Clustering<MyMesh, vcg::tri::AverageColorCell<MyMesh> > Grid;
        //Grid.DuplicateFaceParam = true;
        //Grid.Init(myMesh->bbox, 10000);

        //Grid.AddMesh(*myMesh);
        //Grid.ExtractMesh(*myMesh);
        //printf("origin vertex: %d\n", myMesh->vn);
        //int mergeVertexCount = tri::Clean<MyMesh>::MergeCloseVertex(*myMesh, 0.001 * tileBoxMaxLength);
        //printf("after merge: %d\n", myMesh->vn);

        //tri::UpdateTopology<MyMesh>::FaceVert
        //printf("after split: %d\n", myMesh->vn);
        //std::printf("merge vertex count: %d", mergeVertexCount);
        // decimator initialization
        vcg::LocalOptimization<MyMesh> deciSession(*myMesh, m_pParams);
        clock_t t1 = std::clock();
        deciSession.Init<MyTriEdgeCollapse>();
        clock_t t2 = std::clock();

        // FIXME: If the mesh bbox is large and it's face number is ralatively few, we should not do decimation.
        //uint32_t finalSize = myMesh->fn * tileBoxMaxLength;
        //finalSize = finalSize < MIN_FACE_NUM ? MIN_FACE_NUM : finalSize;
        int targetFaceCount = myMesh->fn * decimationRatio;
        deciSession.SetTargetSimplices(targetFaceCount); // Target face number;
        deciSession.SetTimeBudget(1.5f); // Time budget for each cycle
        deciSession.SetTargetOperations(10000000000);
        int maxTry = 1000000;
        int currentTry = 0;
        do
        {
            deciSession.DoOptimization();
            currentTry++;
        } while (myMesh->fn > targetFaceCount && currentTry < maxTry);
        clock_t t3 = std::clock();
        //if ((t2 - t1) / (float)CLOCKS_PER_SEC > 1.0)
        //{
        //    printf("vertex number: %d\n", myMesh->vn);
        //    printf("face   number: %d\n", myMesh->fn);
        //    printf("init time: %f \n", (t2 - t1) / (float)CLOCKS_PER_SEC);
        //    printf("decimation time: %f\n", (t3 - t2) / (float)CLOCKS_PER_SEC);
        //}

        //if (maxBoxDiag < myMesh->bbox.Diag())
        //{
        //    geometryError = deciSession.currMetric;
        //    maxBoxDiag = myMesh->bbox.Diag();
        //}
        tri::UpdateTopology<MyMesh>::FaceFace(*myMesh);
        //tri::Clean<MyMesh>::SplitNonManifoldVertex(*myMesh, 0.0);
        //tri::Clean<MyMesh>::RemoveNonManifoldFace(*myMesh);
        //tri::Clean<MyMesh>::RemoveNonManifoldVertex(*myMesh);
        //tri::Clean<MyMesh>::RemoveUnreferencedVertex(*myMesh);
        //tri::Clean<MyMesh>::RemoveDegenerateFace(*myMesh);

        tri::Clean<MyMesh>::RemoveUnreferencedVertex(*myMesh);

        totalFaceBeforeSplit += myMesh->fn;
        //printf("after decimation: %d\n", myMesh->vn);
        //tri::UpdateTopology<MyMesh>::FaceFace(*myMesh);
        //tri::UpdateNormal<MyMesh>::PerVertex(*myMesh);
        //tri::UpdateNormal<MyMesh>::PerFace(*myMesh);
        //tri::Clean<MyMesh>::RemoveDegenerateFace(*myMesh);
        //tri::CreaseCut<MyMesh>(*myMesh, math::ToRad(45.0f));
        //printf("after crease: %d\n", myMesh->vn);
        //tri::UpdateNormal<MyMesh>::PerVertexAngleWeighted(*myMesh);
        //tri::UpdateNormal<MyMesh>::PerVertex(*myMesh);
        //tri::UpdateNormal<MyMesh>::NormalizePerVertex(*myMesh);

        totalFaceCount += myMesh->fn;
	}
    printf("tile totalFaceCount before split: %d\n", totalFaceBeforeSplit);
    printf("tile totalFaceCount after Decimation: %d\n", totalFaceCount);
    return tileBoxMaxLength / 16.0f;
}

void MeshOptimizer::DoMerge()
{
	std::unordered_map<tinygltf::Material, MergeMeshInfo, material_hash_fn, material_equal_fn> materialMeshMap;
	for (int i = 0; i < m_meshInfos.size(); ++i)
    {
        if (materialMeshMap.count(*m_meshInfos[i].material) > 0)
        {
            materialMeshMap.at(*m_meshInfos[i].material).meshes.push_back(m_meshInfos[i].myMesh);
        }
        else
        {
            std::vector<MyMesh*> myMeshesToMerge;
            myMeshesToMerge.push_back(m_meshInfos[i].myMesh);
            MergeMeshInfo mergeMeshInfo;
            mergeMeshInfo.meshes = myMeshesToMerge;
            mergeMeshInfo.material = m_meshInfos[i].material;
            materialMeshMap.insert(make_pair(*m_meshInfos[i].material, mergeMeshInfo));
        }
    }

    std::unordered_map<tinygltf::Material, MergeMeshInfo, material_hash_fn, material_equal_fn>::iterator it;
    for (it = materialMeshMap.begin(); it != materialMeshMap.end(); ++it)
    {
        std::vector<MyMesh*> myMeshes = it->second.meshes;

        std::sort(myMeshes.begin(), myMeshes.end(), mesh_compare_fn());

        mergeSameMaterialMeshes(it->second.material, myMeshes);
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
            mergedMesh = new MyMesh();
			m_mergeMeshInfos.push_back(meshInfo);
			totalVertex = 0;
			totalFace = 0;
		}

		ConcatMyMesh(mergedMesh, myMesh);

		totalVertex += myMesh->vn;
		totalFace += myMesh->fn;
	}
}